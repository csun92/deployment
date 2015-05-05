##########################################################################
# MediPy - Copyright (C) Universite de Strasbourg
# Distributed under the terms of the CeCILL-B license, as published by
# the CEA-CNRS-INRIA. Refer to the LICENSE file or to
# http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
# for details.
##########################################################################

import multiprocessing
import os
import sys
import Queue

import wx

import medipy.gui.image
import medipy.io.dicom
import medipy.io.dicom.misc
import medipy.io.dicom.split
import medipy.io.dicom.sort

from hierarchy_tree import HierarchyTree
from dataset_list import DataSetList

class Explorer(wx.Panel):
    
    def __init__(self, parent, ID=wx.ID_ANY, pos=wx.DefaultPosition,
                 size=wx.DefaultSize, style=wx.TAB_TRAVERSAL, name="explorer"):
        
        self._series_position = {}
        self._number_of_frames = {}
        self._loaded_datasets = {}
        self._is_multiframe_image = False
        
        wx.Panel.__init__(self, parent, ID, pos, size, style, name)
        
        # Create widgets
        main_splitter = wx.SplitterWindow(self)
        left_splitter = wx.SplitterWindow(main_splitter)
        self._hierarchy_tree = HierarchyTree(left_splitter)
        bottom_panel = wx.Panel(left_splitter)
        self._slider = wx.Slider(bottom_panel)
        self._image = medipy.gui.image.Image(bottom_panel, "axial")
        self._dataset_list = DataSetList(main_splitter)
        
        # Setup widgets
        self._slider.Disable()
        self._image.use_origin_and_spacing = False
        self._image.orientation_visibility = False
        left_splitter.SplitHorizontally(self._hierarchy_tree, bottom_panel)
        main_splitter.SplitVertically(left_splitter, self._dataset_list) 

        # Layout
        bottom_panel.SetSizer(wx.BoxSizer(wx.VERTICAL))
        bottom_panel.GetSizer().Add(self._slider, flag=wx.EXPAND)
        bottom_panel.GetSizer().Add(self._image, 1, wx.EXPAND)
        self.SetSizer(wx.BoxSizer())
        self.GetSizer().Add(main_splitter, 1, wx.EXPAND)
        
        # Bind events
        self._hierarchy_tree.Bind(wx.EVT_TREE_SEL_CHANGED, self.OnHierarchy)
        self._slider.Bind(wx.EVT_SLIDER, self.OnSlider)
    
    def set_datasets(self, datasets):
        self._hierarchy_tree.set_datasets(datasets)
    
    ##############
    # Properties #
    ##############
    def _get_hierarchy_tree(self):
        return self._hierarchy_tree
    
    hierarchy_tree = property(_get_hierarchy_tree)
    
    ##################
    # Event handlers #
    ##################
    
    def OnHierarchy(self, dummy):
        """ Handler for hierarchy tree selection """

        if self._hierarchy_tree.selected_datasets :
            datasets = self._hierarchy_tree.selected_datasets
            
            self._setup_slider(datasets)
            
            slider_value = self._series_position.get(datasets[0].series_instance_uid.value, 0)
            self._slider.SetValue(slider_value)
            self.OnSlider(None)
        else :
            self._slider.Disable()
    
    def OnSlider(self, dummy):
        """ Handler for slider events """
        
        self.Freeze()
        
        datasets = self._hierarchy_tree.selected_datasets
        series_uid = datasets[0].series_instance_uid.value
        
        loaded_datasets = self._loaded_datasets[series_uid]
        if "number_of_frames" in loaded_datasets[0] :
            dataset = loaded_datasets[0]
            frame_number = self._slider.GetValue()
            slice = slice = dataset.pixel_array[frame_number]
        else :
            dataset = loaded_datasets[self._slider.GetValue()]
            frame_number = None
            slice = dataset.pixel_array
        
        slice = medipy.base.Image(data=slice.reshape((1,)+slice.shape))
        
        if self._image.number_of_layers == 0 :
            self._image.append_layer(slice)
            self._image.reset_view()
        else :
            self._image.delete_layer(0)
            self._image.insert_layer(0, slice)
            self._image.render()
            #self._image.set_layer_image(0, slice)
            # Contrast data
        
        scroll = (self._dataset_list.GetScrollPos(wx.HORIZONTAL), 
                  self._dataset_list.GetScrollPos(wx.VERTICAL))
        self._dataset_list.set_dataset(dataset, frame_number)
        self._dataset_list.SetScrollPos(wx.HORIZONTAL, scroll[0])
        self._dataset_list.SetScrollPos(wx.VERTICAL, scroll[1])
        
        self._series_position[series_uid] = self._slider.GetValue()
        
        self.Thaw()
    
    #####################
    # Private interface #
    #####################
    
    def _setup_slider(self, datasets):
        """ Setup the image slider.
        """
        
        series_uid = datasets[0].series_instance_uid.value
        
        if series_uid not in self._loaded_datasets :
            # Get loadable directory records from the datasets
            records = [x for x in datasets if "directory_record_type" in x]
            file_records = []
            for dataset in datasets :
                if "directory_record_type" not in dataset :
                    continue
                file_records.extend(medipy.io.dicom.misc.get_child_file_records(dataset))
            
            progress_dialog = wx.ProgressDialog(
                "Loading files ...", "Loading files ...", len(file_records))
            
            # Spawn the tasks. Since the loading task is mostly I/O bound, use
            # more tasks than available CPUs.
            if sys.platform.startswith("win"):
                queue = Queue.Queue()
                for record in file_records:
                    _load_directory_record(record, queue)

                loaded_datasets = []
                while len(loaded_datasets) != len(file_records) :
                    loaded_datasets.append(queue.get())
                    # GUI updates must happen in the main thread
                    progress_dialog.Update(len(loaded_datasets))

            else:
                pool = multiprocessing.Pool(2*multiprocessing.cpu_count())
                manager = multiprocessing.Manager()
                queue = manager.Queue()
                tasks_count = 0
                for record in file_records :
                    pool.apply_async(_load_directory_record, (record, queue))

                # Wait for the results...
                loaded_datasets = []
                while len(loaded_datasets) != len(file_records) :
                    loaded_datasets.append(queue.get())
                    # GUI updates must happen in the main thread
                    progress_dialog.Update(len(loaded_datasets))

                # Clean-up pool and progress dialog
                pool.close()
                pool.join()
                progress_dialog.Destroy()
            
            # Add already-loaded datasets (i.e. non-Directory Record)
            loaded_datasets.extend([x for x in datasets if "directory_record_type" not in x])
            
            images = medipy.io.dicom.split.images(loaded_datasets)
            medipy.io.dicom.sort.sort(images)
            
            self._loaded_datasets[series_uid] = images 
        
        loaded_datasets = self._loaded_datasets[series_uid]
        
        slider_max = 0
        for dataset in loaded_datasets :
            if "number_of_frames" in dataset :
                slider_max += dataset.number_of_frames.value
            else :
                slider_max += 1
        
        # When minValue == maxValue, SetRange seems to fail on Linux
        if slider_max == 1 :
            self._slider.Disable()
        else :
            self._slider.Enable()
            self._slider.SetRange(0, slider_max-1)

def _load_directory_record(record, queue):
    """ Worker function used in Explorer._setup_slider. This function returns
        either the loaded dataset or the traceback of any exception that 
        happened.
    
        A "regular" (vs. class)
        function must be use, since bound methods are not pickle-able.
    """
    
    try :
        path = medipy.io.dicom.misc.find_dicomdir_file(
           os.path.dirname(record.path), record.referenced_file_id.value)
        dataset = medipy.io.dicom.read(path)
    except :
        result = sys.exc_info()
    else :
        result = dataset
    queue.put(result)
