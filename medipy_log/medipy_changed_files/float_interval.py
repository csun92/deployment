##########################################################################
# MediPy - Copyright (C) Universite de Strasbourg
# Distributed under the terms of the CeCILL-B license, as published by
# the CEA-CNRS-INRIA. Refer to the LICENSE file or to
# http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html
# for details.
##########################################################################

import math

import wx
import wx.xrc

from medipy.base import Observable

class FloatInterval(wx.PyPanel, Observable) :
    """ Control to choose a value from a float interval.
    """
    
    def __init__(self, parent, id=wx.ID_ANY, min_max=(0, 100), value=(20,80),
                 orientation = wx.VERTICAL, pos=wx.DefaultPosition, size=wx.DefaultSize, 
                 style=wx.TAB_TRAVERSAL|wx.NO_BORDER, name=wx.PanelNameStr) :
        
        self._range = min_max
        self._value = None
        self._orientation = None
        
        self._triangle_size = None
        self._line = None
        self._min_triangle = None
        self._max_triangle = None
        
        # Name of the cursor that will be moved by the mouse, or None
        self._moving_cursor = None
        
        wx.PyPanel.__init__(self, parent, id, pos, size, style, name)
        Observable.__init__(self, ["value"])
        
        point_size = self.GetFont().GetPointSize()
        
        self._panel = wx.Panel(self)
        self._min_text = wx.TextCtrl(self, size=(point_size*10, -1))
        self._max_text = wx.TextCtrl(self, size=(point_size*10, -1))
        
        self._triangle_size = point_size*1.2
        
        self._set_orientation(orientation)
        
        self._panel.Bind(wx.EVT_LEFT_DOWN, self.OnLeftClick)
        self._panel.Bind(wx.EVT_LEFT_UP, self.OnLeftRelease)
        self._panel.Bind(wx.EVT_SIZE, self.OnSize)
        self._panel.Bind(wx.EVT_MOTION, self.OnMouseMove)
        self._panel.Bind(wx.EVT_PAINT, self.OnPaint)
        self._panel.Bind(wx.EVT_ERASE_BACKGROUND, self.OnEraseBackground)
        self._min_text.Bind(wx.EVT_TEXT, self.OnText)
        self._max_text.Bind(wx.EVT_TEXT, self.OnText)
        
        self._set_value(value)
    
    def validate(self) :
        return self._value[0]<=self._value[1]
    
    def reset(self):
        self._set_value(self._range)
    
    def DoGetBestSize(self):
        point_size = self.GetFont().GetPointSize()
        if self._orientation == wx.VERTICAL :
            totalWidth = point_size*10
            totalHeight = 10*self._triangle_size+2*self._min_text.GetBestSize()[1]
        else :
            totalWidth = 10*self._triangle_size+2*point_size*10
            totalHeight = self._min_text.GetBestSize()[1]
        best = wx.Size(totalWidth, totalHeight)

        # Cache the best size so it doesn't need to be calculated again,
        # at least until some properties of the window change
        self.CacheBestSize(best)

        return best
    
    ##############
    # Properties #
    ##############
    
    def _get_value(self) :
        return self._value
    
    def _set_value(self, value) :
        self._value = value
        self.notify_observers("value")
        self.Refresh()
    
    def _get_range(self):
        return self._range
    
    def _set_range(self, range):
        self._range = range
        self.Refresh()
    
    def _get_orientation(self):
        return self._orientation
    
    def _set_orientation(self, orientation):
        
        if self._orientation is not None :
            self.GetSizer().Clear()
        
        self._orientation = orientation
        
        if self._orientation == wx.VERTICAL :
            sizer = wx.BoxSizer(wx.VERTICAL)
            
            sizer.Add(self._max_text, 0, wx.ALIGN_CENTER)
            sizer.Add(self._panel, 1, wx.ALIGN_CENTER)
            sizer.Add(self._min_text, 0, wx.ALIGN_CENTER)
        else : 
            sizer = wx.BoxSizer(wx.HORIZONTAL)
            
            sizer.Add(self._min_text, 0, wx.ALIGN_CENTER)
            sizer.Add(self._panel, 1, wx.ALIGN_CENTER)
            sizer.Add(self._max_text, 0, wx.ALIGN_CENTER)
        self.SetSizer(sizer)
        self.Layout()
        self.DoGetBestSize()
    
    value = property(_get_value, _set_value)
    range = property(_get_range, _set_range)
    orientation = property(_get_orientation, _set_orientation)
    
    ##################
    # Event handlers #
    ##################
    
    def OnLeftClick(self, event) :
        position = event.GetPosition()
        
        # Figure on which side of the line the user clicked
        width, height = self._panel.GetClientSize()
        if self._orientation == wx.VERTICAL :
            self._moving_cursor = "min" if (position[0] < width/2) else "max"
        else : 
            self._moving_cursor = "min" if (position[1] < height/2) else "max"
        
        self._set_value_from_screen_position(position)
    
    def OnLeftRelease(self, dummy):
        self._moving_cursor = None
    
    def OnMouseMove(self, event) :
        if event.LeftIsDown() :
            self._set_value_from_screen_position(event.GetPosition())
    
    def OnPaint(self, dummy) :
        dc = wx.BufferedPaintDC(self._panel)
        self._draw(dc)
        index = self._min_text.GetInsertionPoint()
        self._min_text.ChangeValue("%f"%self._value[0])
        self._min_text.SetInsertionPoint(index)
        
        index = self._max_text.GetInsertionPoint()
        self._max_text.ChangeValue("%f"%self._value[1])
        self._max_text.SetInsertionPoint(index)
    
    def OnEraseBackground(self, dummy):
        """ Handles the wx.EVT_ERASE_BACKGROUND event for CustomCheckBox. """
        # This is intentionally empty, because we are using the combination
        # of wx.BufferedPaintDC + an empty OnEraseBackground event to
        # reduce flicker
        pass
    
    def OnSize(self, dummy) :
        self.Refresh()

    def OnText(self, event) :
        # Validate object
        control = event.GetEventObject()
        try :
            value = float(control.GetValue())
        except ValueError :
            control.SetBackgroundColour(wx.RED)
        else : 
            
            value = (float(self._min_text.GetValue()), 
                     float(self._max_text.GetValue()))
            if value[0] <= value[1] :
                self._min_text.SetBackgroundColour(None)
                self._max_text.SetBackgroundColour(None)
                self._set_value((float(self._min_text.GetValue()), 
                                 float(self._max_text.GetValue())))
                self.Refresh()
            else :
                self._min_text.SetBackgroundColour(wx.RED)
                self._max_text.SetBackgroundColour(wx.RED)

    #####################
    # Private interface #
    #####################
    
    def _compute_ui(self, width, height) :
        if self._orientation == wx.VERTICAL :
            self._line = ((width/2, self._triangle_size), 
                          (width/2, height-self._triangle_size))
            screen_length = self._line[1][1]-self._line[0][1]
        else :
            self._line = ((self._triangle_size, height/2), 
                          (width-self._triangle_size, height/2))
            screen_length = self._line[1][0]-self._line[0][0]
        
        real_length = float(self._range[1]-self._range[0])
        
        # Control range, normalized between 0 and 1
        if real_length != 0 :
            normalized_range = ((self._value[0]-self._range[0])/real_length,
                                (self._value[1]-self._range[0])/real_length)
        else :
            normalized_range = (0,0)
        
        # Range in screen coordinates
        if self._orientation == wx.VERTICAL :
            screen_range = (self._line[1][1]-normalized_range[0]*screen_length,
                            self._line[1][1]-normalized_range[1]*screen_length)
        else :
            screen_range = (self._line[0][0]+normalized_range[0]*screen_length,
                            self._line[0][0]+normalized_range[1]*screen_length)
        
        # Triangles
        side = math.sin(math.pi/3)*self._triangle_size
        if self._orientation == wx.VERTICAL :
            min_start = (width/2, screen_range[0])
            max_start = (width/2, screen_range[1])
            self._min_triangle = [min_start,  
                            (min_start[0]-side, min_start[1]-self._triangle_size/2),
                            (min_start[0]-side, min_start[1]+self._triangle_size/2)]
            self._max_triangle = [max_start,  
                            (max_start[0]+side, max_start[1]-self._triangle_size/2),
                            (max_start[0]+side, max_start[1]+self._triangle_size/2)]
        else :
            min_start = (screen_range[0], height/2)
            max_start = (screen_range[1], height/2)
            self._min_triangle = [min_start,  
                            (min_start[0]-self._triangle_size/2, min_start[1]-side),
                            (min_start[0]+self._triangle_size/2, min_start[1]-side)]
            self._max_triangle=[max_start,  
                          (max_start[0]-self._triangle_size/2, max_start[1]+side),
                          (max_start[0]+self._triangle_size/2, max_start[1]+side)]
    
    def _draw(self, dc) :
        # Get the actual client size of ourselves
        width, height = self._panel.GetClientSize()
        if not width or not height:
            # Nothing to do, we still don't have dimensions!
            return
        
        # Erase the control
        background_brush = wx.Brush(self.GetBackgroundColour())
        dc.SetBackground(background_brush)
        dc.Clear()
        
        self._compute_ui(width, height)
        
        line_brush = wx.Brush(wx.Colour(0,0,0))
        line_pen = wx.Pen(wx.Colour(0, 0, 0))
        dc.SetBrush(line_brush)
        dc.SetPen(line_pen)
        dc.DrawLine(self._line[0][0], self._line[0][1], 
                    self._line[1][0], self._line[1][1])
        
        min_brush = wx.Brush(wx.Colour(48,48,203))
        min_pen = wx.Pen(wx.Colour(36, 36, 153))
        
        max_brush = wx.Brush(wx.Colour(203,48,48))
        max_pen = wx.Pen(wx.Colour(153, 36, 36))
        
        dc.SetBrush(min_brush)
        dc.SetPen(min_pen)
        dc.DrawPolygon(self._min_triangle)
        
        dc.SetBrush(max_brush)
        dc.SetPen(max_pen)
        dc.DrawPolygon(self._max_triangle)

    def _set_value_from_screen_position(self, position) :
        width, height = self._panel.GetClientSize()
        
        if not width or not height:
            # Nothing to do, we still don't have dimensions!
            return
        
        # Compute the new value
        if self._orientation == wx.VERTICAL :
            screen_length = self._line[1][1]-self._line[0][1]
            normalized = (self._line[1][1]-position[1])/float(screen_length)
        else :
            screen_length = self._line[1][0]-self._line[0][0]
            normalized = (position[0]-self._line[0][0])/float(screen_length)
        
        clamped = max(0., min(1., normalized))
        value = self._range[0]+clamped*(self._range[1]-self._range[0])
        
        if self._moving_cursor == "min" : 
            value = min(self._value[1], value)
            self._set_value([value, self._value[1]])
        elif self._moving_cursor == "max" :
            value = max(self._value[0], value)
            self._set_value([self._value[0], value])
