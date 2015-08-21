"""

A VTK RenderWindowInteractor widget for wxPython.

Find wxPython info at http://wxPython.org

Created by Prabhu Ramachandran, April 2002
Based on wxVTKRenderWindow.py

Fixes and updates by Charl P. Botha 2003-2008

Updated to new wx namespace and some cleaning up by Andrea Gavana,
December 2006
"""

"""
Please see the example at the end of this file.

----------------------------------------
Creation:

 wxVTKRenderWindowInteractor(parent, ID, stereo=0, [wx keywords]):

 You should create a wx.PySimpleApp() or some other wx**App before
 creating the window.

Behaviour:

 Uses __getattr__ to make the wxVTKRenderWindowInteractor behave just
 like a vtkGenericRenderWindowInteractor.

----------------------------------------

"""

# import usual libraries
import wx
import vtk

# wxPython 2.4.0.4 and newer prefers the use of True and False, standard
# booleans in Python 2.2 but not earlier.  Here we define these values if
# they don't exist so that we can use True and False in the rest of the
# code.  At the time of this writing, that happens exactly ONCE in
# CreateTimer()
try:
    True
except NameError:
    True = 1
    False = 0

# a few configuration items, see what works best on your system

# Use GLCanvas as base class instead of wx.Window.
# This is sometimes necessary under wxGTK or the image is blank.
# (in wxWindows 2.3.1 and earlier, the GLCanvas had scroll bars)
baseClass = wx.Window
if wx.Platform == "__WXGTK__":
    import wx.glcanvas
    baseClass = wx.glcanvas.GLCanvas

# Keep capturing mouse after mouse is dragged out of window
# (in wxGTK 2.3.2 there is a bug that keeps this from working,
# but it is only relevant in wxGTK if there are multiple windows)
_useCapture = (wx.Platform == "__WXMSW__")

# end of configuration items


class EventTimer(wx.Timer):
    """Simple wx.Timer class.
    """

    def __init__(self, iren):
        """Default class constructor.
        @param iren: current render window
        """
        wx.Timer.__init__(self)
        self.iren = iren


    def Notify(self):
        """ The timer has expired.
        """
        self.iren.TimerEvent()


class wxVTKRenderWindowInteractor(baseClass):
    """
    A wxRenderWindow for wxPython.
    Use GetRenderWindow() to get the vtkRenderWindow.
    Create with the keyword stereo=1 in order to
    generate a stereo-capable window.
    """

    keysyms = {
#        wx.WXK_ADD : " 335
#        wx.WXK_ALT : " 307
        wx.WXK_BACK : "BackSpace",
#        wx.WXK_CANCEL : " 303
#        wx.WXK_CAPITAL : " 311
#        wx.WXK_CLEAR : " 305
#        wx.WXK_COMMAND : " 396
#        wx.WXK_CONTROL : " 308
#        wx.WXK_DECIMAL : " 338
        wx.WXK_DELETE : "Delete",
#        wx.WXK_DIVIDE : " 339
        wx.WXK_DOWN : "Down",
        wx.WXK_END : "End",
        wx.WXK_ESCAPE : "Escape",
#        wx.WXK_EXECUTE : " 320
        wx.WXK_F1 : "F1",
        wx.WXK_F10 : "F10",
        wx.WXK_F11 : "F11",
        wx.WXK_F12 : "F12",
#        wx.WXK_F13 : " 352
#        wx.WXK_F14 : " 353
#        wx.WXK_F15 : " 354
#        wx.WXK_F16 : " 355
#        wx.WXK_F17 : " 356
#        wx.WXK_F18 : " 357
#        wx.WXK_F19 : " 358
        wx.WXK_F2 : "F2",
#        wx.WXK_F20 : " 359
#        wx.WXK_F21 : " 360
#        wx.WXK_F22 : " 361
#        wx.WXK_F23 : " 362
#        wx.WXK_F24 : " 363
        wx.WXK_F3 : "F3",
        wx.WXK_F4 : "F4",
        wx.WXK_F5 : "F5",
        wx.WXK_F6 : "F6",
        wx.WXK_F7 : "F7",
        wx.WXK_F8 : "F8",
        wx.WXK_F9 : "F9",
#        wx.WXK_HELP : " 323
        wx.WXK_HOME : "Home",
        wx.WXK_INSERT : "Insert",
#        wx.WXK_LBUTTON : " 301
        wx.WXK_LEFT : "Left",
#        wx.WXK_MBUTTON : " 304
#        wx.WXK_MENU : " 309
#        wx.WXK_MULTIPLY : " 334
        wx.WXK_NEXT : "Next",
#        wx.WXK_NUMLOCK : " 364
#        wx.WXK_NUMPAD0 : " 324
#        wx.WXK_NUMPAD1 : " 325
#        wx.WXK_NUMPAD2 : " 326
#        wx.WXK_NUMPAD3 : " 327
#        wx.WXK_NUMPAD4 : " 328
#        wx.WXK_NUMPAD5 : " 329
#        wx.WXK_NUMPAD6 : " 330
#        wx.WXK_NUMPAD7 : " 331
#        wx.WXK_NUMPAD8 : " 332
#        wx.WXK_NUMPAD9 : " 333
#        wx.WXK_NUMPAD_ADD : " 388
#        wx.WXK_NUMPAD_BEGIN : " 383
#        wx.WXK_NUMPAD_DECIMAL : " 391
#        wx.WXK_NUMPAD_DELETE : " 385
#        wx.WXK_NUMPAD_DIVIDE : " 392
#        wx.WXK_NUMPAD_DOWN : " 379
#        wx.WXK_NUMPAD_END : " 382
#        wx.WXK_NUMPAD_ENTER : " 370
#        wx.WXK_NUMPAD_EQUAL : " 386
#        wx.WXK_NUMPAD_F1 : " 371
#        wx.WXK_NUMPAD_F2 : " 372
#        wx.WXK_NUMPAD_F3 : " 373
#        wx.WXK_NUMPAD_F4 : " 374
#        wx.WXK_NUMPAD_HOME : " 375
#        wx.WXK_NUMPAD_INSERT : " 384
#        wx.WXK_NUMPAD_LEFT : " 376
#        wx.WXK_NUMPAD_MULTIPLY : " 387
#        wx.WXK_NUMPAD_NEXT : " 381
#        wx.WXK_NUMPAD_PAGEDOWN : " 381
#        wx.WXK_NUMPAD_PAGEUP : " 380
#        wx.WXK_NUMPAD_PRIOR : " 380
#        wx.WXK_NUMPAD_RIGHT : " 378
#        wx.WXK_NUMPAD_SEPARATOR : " 389
#        wx.WXK_NUMPAD_SPACE : " 368
#        wx.WXK_NUMPAD_SUBTRACT : " 390
#        wx.WXK_NUMPAD_TAB : " 369
#        wx.WXK_NUMPAD_UP : " 377
        wx.WXK_PAGEDOWN : "PageDown",
        wx.WXK_PAGEUP : "PageUp",
        wx.WXK_PAUSE : "Pause",
#        wx.WXK_PRINT : " 319
        wx.WXK_PRIOR : "Prior",
#        wx.WXK_RBUTTON : " 302
        wx.WXK_RETURN : "Return",
        wx.WXK_RIGHT : "Right",
#        wx.WXK_SCROLL : " 365
#        wx.WXK_SELECT : " 318
#        wx.WXK_SEPARATOR : " 336
#        wx.WXK_SHIFT : " 306
#        wx.WXK_SNAPSHOT : " 321
#        wx.WXK_SPACE : " 32
#        wx.WXK_SPECIAL1 : " 193
#        wx.WXK_SPECIAL10 : " 202
#        wx.WXK_SPECIAL11 : " 203
#        wx.WXK_SPECIAL12 : " 204
#        wx.WXK_SPECIAL13 : " 205
#        wx.WXK_SPECIAL14 : " 206
#        wx.WXK_SPECIAL15 : " 207
#        wx.WXK_SPECIAL16 : " 208
#        wx.WXK_SPECIAL17 : " 209
#        wx.WXK_SPECIAL18 : " 210
#        wx.WXK_SPECIAL19 : " 211
#        wx.WXK_SPECIAL2 : " 194
#        wx.WXK_SPECIAL20 : " 212
#        wx.WXK_SPECIAL3 : " 195
#        wx.WXK_SPECIAL4 : " 196
#        wx.WXK_SPECIAL5 : " 197
#        wx.WXK_SPECIAL6 : " 198
#        wx.WXK_SPECIAL7 : " 199
#        wx.WXK_SPECIAL8 : " 200
#        wx.WXK_SPECIAL9 : " 201
#        wx.WXK_START : " 300
#        wx.WXK_SUBTRACT : " 337
        wx.WXK_TAB : "Tab",
        wx.WXK_UP : "Up",
#        wx.WXK_WINDOWS_LEFT : " 393
#        wx.WXK_WINDOWS_MENU : " 395
#        wx.WXK_WINDOWS_RIGHT : " 394
    }


    # class variable that can also be used to request instances that use
    # stereo; this is overridden by the stereo=1/0 parameter.  If you set
    # it to True, the NEXT instantiated object will attempt to allocate a
    # stereo visual.  E.g.:
    # wxVTKRenderWindowInteractor.USE_STEREO = True
    # myRWI = wxVTKRenderWindowInteractor(parent, -1)
    USE_STEREO = False

    def __init__(self, parent, ID, *args, **kw):
        """Default class constructor.
        @param parent: parent window
        @param ID: window id
        @param **kw: wxPython keywords (position, size, style) plus the
        'stereo' keyword
        """
        # private attributes
        self.__RenderWhenDisabled = 0

        # First do special handling of some keywords:
        # stereo, position, size, style

        stereo = 0

        if kw.has_key('stereo'):
            if kw['stereo']:
                stereo = 1
            del kw['stereo']

        elif self.USE_STEREO:
            stereo = 1

        position, size = wx.DefaultPosition, wx.DefaultSize

        if kw.has_key('position'):
            position = kw['position']
            del kw['position']

        if kw.has_key('size'):
            size = kw['size']
            del kw['size']

        # wx.WANTS_CHARS says to give us e.g. TAB
        # wx.NO_FULL_REPAINT_ON_RESIZE cuts down resize flicker under GTK
        style = wx.WANTS_CHARS | wx.NO_FULL_REPAINT_ON_RESIZE

        if kw.has_key('style'):
            style = style | kw['style']
            del kw['style']

        # the enclosing frame must be shown under GTK or the windows
        #  don't connect together properly
        if wx.Platform != '__WXMSW__':
            l = []
            p = parent
            while p: # make a list of all parents
                l.append(p)
                p = p.GetParent()
            l.reverse() # sort list into descending order
            for p in l:
                p.Show(1)

        if baseClass.__name__ == 'GLCanvas':
            # code added by cpbotha to enable stereo and double
            # buffering correctly where the user requests this; remember
            # that the glXContext in this case is NOT allocated by VTK,
            # but by WX, hence all of this.

            # Initialize GLCanvas with correct attriblist
            attribList = [wx.glcanvas.WX_GL_RGBA,
                          wx.glcanvas.WX_GL_MIN_RED, 1,
                          wx.glcanvas.WX_GL_MIN_GREEN, 1,
                          wx.glcanvas.WX_GL_MIN_BLUE, 1,
                          wx.glcanvas.WX_GL_DEPTH_SIZE, 16,
                          wx.glcanvas.WX_GL_DOUBLEBUFFER]
            if stereo:
                attribList.append(wx.glcanvas.WX_GL_STEREO)

            try:
                baseClass.__init__(
                    self, parent, ID, pos=position, size=size, style=style,
                    attribList=attribList)
            except wx.PyAssertionError:
                # visual couldn't be allocated, so we go back to default
                baseClass.__init__(self, parent, ID, position, size, style)
                if stereo:
                    # and make sure everyone knows that the stereo
                    # visual wasn't set.
                    stereo = 0

        else:
            baseClass.__init__(self, parent, ID, position, size, style)

        # create the RenderWindow and initialize it
        self._Iren = vtk.vtkGenericRenderWindowInteractor()
        self._Iren.SetRenderWindow( vtk.vtkRenderWindow() )
        self._Iren.AddObserver('CreateTimerEvent', self.CreateTimer)
        self._Iren.AddObserver('DestroyTimerEvent', self.DestroyTimer)
        self._Iren.GetRenderWindow().AddObserver('CursorChangedEvent',
                                                 self.CursorChangedEvent)

        try:
            self._Iren.GetRenderWindow().SetSize(size.width, size.height)
        except AttributeError:
            self._Iren.GetRenderWindow().SetSize(size[0], size[1])

        if stereo:
            self._Iren.GetRenderWindow().StereoCapableWindowOn()
            self._Iren.GetRenderWindow().SetStereoTypeToCrystalEyes()

        self.__handle = None

        self.BindEvents()

        # with this, we can make sure that the reparenting logic in
        # Render() isn't called before the first OnPaint() has
        # successfully been run (and set up the VTK/WX display links)
        self.__has_painted = False

        # set when we have captured the mouse.
        self._own_mouse = False
        # used to store WHICH mouse button led to mouse capture
        self._mouse_capture_button = 0

        # A mapping for cursor changes.
        self._cursor_map = {0: wx.CURSOR_ARROW, # VTK_CURSOR_DEFAULT
                            1: wx.CURSOR_ARROW, # VTK_CURSOR_ARROW
                            2: wx.CURSOR_SIZENESW, # VTK_CURSOR_SIZENE
                            3: wx.CURSOR_SIZENWSE, # VTK_CURSOR_SIZENWSE
                            4: wx.CURSOR_SIZENESW, # VTK_CURSOR_SIZESW
                            5: wx.CURSOR_SIZENWSE, # VTK_CURSOR_SIZESE
                            6: wx.CURSOR_SIZENS, # VTK_CURSOR_SIZENS
                            7: wx.CURSOR_SIZEWE, # VTK_CURSOR_SIZEWE
                            8: wx.CURSOR_SIZING, # VTK_CURSOR_SIZEALL
                            9: wx.CURSOR_HAND, # VTK_CURSOR_HAND
                            10: wx.CURSOR_CROSS, # VTK_CURSOR_CROSSHAIR
                           }

    def BindEvents(self):
        """Binds all the necessary events for navigation, sizing,
        drawing.
        """
        # refresh window by doing a Render
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        # turn off background erase to reduce flicker
        self.Bind(wx.EVT_ERASE_BACKGROUND, lambda e: None)

        # Bind the events to the event converters
        self.Bind(wx.EVT_RIGHT_DOWN, self.OnButtonDown)
        self.Bind(wx.EVT_LEFT_DOWN, self.OnButtonDown)
        self.Bind(wx.EVT_MIDDLE_DOWN, self.OnButtonDown)
        self.Bind(wx.EVT_RIGHT_UP, self.OnButtonUp)
        self.Bind(wx.EVT_LEFT_UP, self.OnButtonUp)
        self.Bind(wx.EVT_MIDDLE_UP, self.OnButtonUp)
        self.Bind(wx.EVT_MOUSEWHEEL, self.OnMouseWheel)
        self.Bind(wx.EVT_MOTION, self.OnMotion)

        self.Bind(wx.EVT_ENTER_WINDOW, self.OnEnter)
        self.Bind(wx.EVT_LEAVE_WINDOW, self.OnLeave)

        # If we use EVT_KEY_DOWN instead of EVT_CHAR, capital versions
        # of all characters are always returned.  EVT_CHAR also performs
        # other necessary keyboard-dependent translations.
        self.Bind(wx.EVT_CHAR, self.OnKeyDown)
        self.Bind(wx.EVT_KEY_UP, self.OnKeyUp)

        self.Bind(wx.EVT_SIZE, self.OnSize)

        # the wx 2.8.7.1 documentation states that you HAVE to handle
        # this event if you make use of CaptureMouse, which we do.
        if _useCapture and hasattr(wx, 'EVT_MOUSE_CAPTURE_LOST'):
            self.Bind(wx.EVT_MOUSE_CAPTURE_LOST,
                    self.OnMouseCaptureLost)

        self.Bind(wx.EVT_CLOSE, self.OnClose)


    def __getattr__(self, attr):
        """Makes the object behave like a
        vtkGenericRenderWindowInteractor.
        """
        if attr == '__vtk__':
            return lambda t=self._Iren: t
        elif hasattr(self._Iren, attr):
            return getattr(self._Iren, attr)
        else:
            raise AttributeError, self.__class__.__name__ + \
                  " has no attribute named " + attr

    def CreateTimer(self, obj, evt):
        """ Creates a timer.
        """
        self._timer = EventTimer(self)
        self._timer.Start(10, True)

    def DestroyTimer(self, obj, evt):
        """The timer is a one shot timer so will expire automatically.
        """
        return 1

    def _CursorChangedEvent(self, obj, evt):
        """Change the wx cursor if the renderwindow's cursor was
        changed.
        """
        cur = self._cursor_map[obj.GetCurrentCursor()]
        c = wx.StockCursor(cur)
        self.SetCursor(c)

    def CursorChangedEvent(self, obj, evt):
        """Called when the CursorChangedEvent fires on the render
        window."""
        # This indirection is needed since when the event fires, the
        # current cursor is not yet set so we defer this by which time
        # the current cursor should have been set.
        wx.CallAfter(self._CursorChangedEvent, obj, evt)

    def HideCursor(self):
        """Hides the cursor."""
        c = wx.StockCursor(wx.CURSOR_BLANK)
        self.SetCursor(c)

    def ShowCursor(self):
        """Shows the cursor."""
        rw = self._Iren.GetRenderWindow()
        cur = self._cursor_map[rw.GetCurrentCursor()]
        c = wx.StockCursor(cur)
        self.SetCursor(c)

    def GetDisplayId(self):
        """Function to get X11 Display ID from WX and return it in a format
        that can be used by VTK Python.

        We query the X11 Display with a new call that was added in wxPython
        2.6.0.1.  The call returns a SWIG object which we can query for the
        address and subsequently turn into an old-style SWIG-mangled string
        representation to pass to VTK.
        """
        d = None

        try:
            d = wx.GetXDisplay()

        except NameError:
            # wx.GetXDisplay was added by Robin Dunn in wxPython 2.6.0.1
            # if it's not available, we can't pass it.  In general,
            # things will still work; on some setups, it'll break.
            pass

        else:
            # wx returns None on platforms where wx.GetXDisplay is not relevant
            if d:
                d = hex(d)
                # On wxPython-2.6.3.2 and above there is no leading '0x'.
                if not d.startswith('0x'):
                    d = '0x' + d

                # we now have 0xdeadbeef
                # VTK wants it as: _deadbeef_void_p (pre-SWIG-1.3 style)
                d = '_%s_%s' % (d[2:], 'void_p')

        return d

    def Enable(self, value):
        """ Enable (if value==True, or Disable if value==False) both the wx
            widget and the VTK interactor.
        """

        super(wxVTKRenderWindowInteractor, self).Enable(value)
        if value :
            self._Iren.Enable()
        else :
            self._Iren.Disable()

    def Disable(self):
        """ Disable both the wx widget and the VTK interactor.
        """

        self.Enable(False)

    def OnMouseCaptureLost(self, event):
        """This is signalled when we lose mouse capture due to an
        external event, such as when a dialog box is shown.  See the
        wx documentation.
        """

        # the documentation seems to imply that by this time we've
        # already lost capture.  I have to assume that we don't need
        # to call ReleaseMouse ourselves.
        if _useCapture and self._own_mouse:
            self._own_mouse = False

    def OnPaint(self,event):
        """Handles the wx.EVT_PAINT event for
        wxVTKRenderWindowInteractor.
        """

        # wx should continue event processing after this handler.
        # We call this BEFORE Render(), so that if Render() raises
        # an exception, wx doesn't re-call OnPaint repeatedly.
        event.Skip()

        dc = wx.PaintDC(self)

        # make sure the RenderWindow is sized correctly
        self._Iren.GetRenderWindow().SetSize(self.GetSizeTuple()[0], self.GetSizeTuple()[1])

        # Tell the RenderWindow to render inside the wx.Window.
        if not self.__handle:

            # on relevant platforms, set the X11 Display ID
            d = self.GetDisplayId()
            if d:
                self._Iren.GetRenderWindow().SetDisplayId(d)

            # store the handle
            self.__handle = self.GetHandle()
            # and give it to VTK
            self._Iren.GetRenderWindow().SetWindowInfo(str(self.__handle))

            # now that we've painted once, the Render() reparenting logic
            # is safe
            self.__has_painted = True

        self.Render()

    def OnSize(self,event):
        """Handles the wx.EVT_SIZE event for
        wxVTKRenderWindowInteractor.
        """

        # event processing should continue (we call this before the
        # Render(), in case it raises an exception)
        event.Skip()

        try:
            width, height = event.GetSize()
        except:
            width = event.GetSize().width
            height = event.GetSize().height
        self._Iren.UpdateSize(width, height)
        self._Iren.ConfigureEvent()

        # this will check for __handle
        self.Render()

    def OnMotion(self,event):
        """Handles the wx.EVT_MOTION event for
        wxVTKRenderWindowInteractor.
        """

        # event processing should continue
        # we call this early in case any of the VTK code raises an
        # exception.
        event.Skip()

        self._Iren.SetEventInformationFlipY(event.GetX(), event.GetY(),
                                            event.ControlDown(),
                                            event.ShiftDown(),
                                            chr(0), 0, None)
        self._Iren.MouseMoveEvent()

    def OnEnter(self,event):
        """Handles the wx.EVT_ENTER_WINDOW event for
        wxVTKRenderWindowInteractor.
        """

        # event processing should continue
        event.Skip()

        self._Iren.SetEventInformationFlipY(event.GetX(), event.GetY(),
                                            event.ControlDown(),
              event.ShiftDown(),
              chr(0), 0, None)
        self._Iren.EnterEvent()


    def OnLeave(self,event):
        """Handles the wx.EVT_LEAVE_WINDOW event for
        wxVTKRenderWindowInteractor.
        """

        # event processing should continue
        event.Skip()

        self._Iren.SetEventInformationFlipY(event.GetX(), event.GetY(),
                                            event.ControlDown(),
              event.ShiftDown(),
              chr(0), 0, None)
        self._Iren.LeaveEvent()


    def OnButtonDown(self,event):
        """Handles the wx.EVT_LEFT/RIGHT/MIDDLE_DOWN events for
        wxVTKRenderWindowInteractor.
        """

        # allow wx event processing to continue
        # on wxPython 2.6.0.1, omitting this will cause problems with
        # the initial focus, resulting in the wxVTKRWI ignoring keypresses
        # until we focus elsewhere and then refocus the wxVTKRWI frame
        # we do it this early in case any of the following VTK code
        # raises an exception.
        event.Skip()

        ctrl, shift = event.ControlDown(), event.ShiftDown()
        self._Iren.SetEventInformationFlipY(event.GetX(), event.GetY(),
                                            ctrl, shift, chr(0), 0, None)

        button = 0
        if event.RightDown():
            self._Iren.RightButtonPressEvent()
            button = 'Right'
        elif event.LeftDown():
            self._Iren.LeftButtonPressEvent()
            button = 'Left'
        elif event.MiddleDown():
            self._Iren.MiddleButtonPressEvent()
            button = 'Middle'

        # save the button and capture mouse until the button is released
        # we only capture the mouse if it hasn't already been captured
        if _useCapture and not self._own_mouse:
            self._own_mouse = True
            self._mouse_capture_button = button
            self.CaptureMouse()


    def OnButtonUp(self,event):
        """Handles the wx.EVT_LEFT/RIGHT/MIDDLE_UP events for
        wxVTKRenderWindowInteractor.
        """

        # event processing should continue
        event.Skip()

        button = 0
        if event.RightUp():
            button = 'Right'
        elif event.LeftUp():
            button = 'Left'
        elif event.MiddleUp():
            button = 'Middle'

        # if the same button is released that captured the mouse, and
        # we have the mouse, release it.
        # (we need to get rid of this as soon as possible; if we don't
        #  and one of the event handlers raises an exception, mouse
        #  is never released.)
        if _useCapture and self._own_mouse and \
                button==self._mouse_capture_button:
            self.ReleaseMouse()
            self._own_mouse = False

        ctrl, shift = event.ControlDown(), event.ShiftDown()
        self._Iren.SetEventInformationFlipY(event.GetX(), event.GetY(),
                                            ctrl, shift, chr(0), 0, None)

        if button == 'Right':
            self._Iren.RightButtonReleaseEvent()
        elif button == 'Left':
            self._Iren.LeftButtonReleaseEvent()
        elif button == 'Middle':
            self._Iren.MiddleButtonReleaseEvent()


    def OnMouseWheel(self,event):
        """Handles the wx.EVT_MOUSEWHEEL event for
        wxVTKRenderWindowInteractor.
        """

        # event processing should continue
        event.Skip()

        ctrl, shift = event.ControlDown(), event.ShiftDown()
        self._Iren.SetEventInformationFlipY(event.GetX(), event.GetY(),
                                            ctrl, shift, chr(0), 0, None)
        if event.GetWheelRotation() > 0:
            self._Iren.MouseWheelForwardEvent()
        else:
            self._Iren.MouseWheelBackwardEvent()

    def OnKeyDown(self,event):
        """Handles the wx.EVT_KEY_DOWN event for wxVTKRenderWindowInteractor.
        """

        # event processing should continue
        event.Skip()
        self._set_key_event_informations(event)
        self._Iren.KeyPressEvent()
        self._Iren.CharEvent()

    def OnKeyUp(self,event):
        """Handles the wx.EVT_KEY_UP event for wxVTKRenderWindowInteractor.
        """

        # event processing should continue
        event.Skip()
        self._set_key_event_informations(event)
        self._Iren.KeyReleaseEvent()

    def OnClose(self, event):
        if self.GetRenderWindow() is not None :
            self.GetRenderWindow().Finalize()
            renderers = self.GetRenderWindow().GetRenderers()
            renderers.InitTraversal()
            renderer = renderers.GetNextItem()
            while renderer is not None :
                renderer.RemoveAllViewProps()
                renderer = renderers.GetNextItem()
        event.Skip()

    def GetRenderWindow(self):
        """Returns the render window (vtkRenderWindow).
        """
        return self._Iren.GetRenderWindow()

    def Render(self):
        """Actually renders the VTK scene on screen.
        """
        RenderAllowed = 1

        if not self.__RenderWhenDisabled:
            # the user doesn't want us to render when the toplevel frame
            # is disabled - first find the top level parent
            topParent = wx.GetTopLevelParent(self)
            if topParent:
                # if it exists, check whether it's enabled
                # if it's not enabeld, RenderAllowed will be false
                RenderAllowed = topParent.IsEnabled()

        if RenderAllowed:
            if self.__handle and self.__handle == self.GetHandle():
                self._Iren.GetRenderWindow().Render()

            elif self.GetHandle() and self.__has_painted:
                # this means the user has reparented us; let's adapt to the
                # new situation by doing the WindowRemap dance
                self._Iren.GetRenderWindow().SetNextWindowInfo(
                    str(self.GetHandle()))

                # make sure the DisplayId is also set correctly
                d = self.GetDisplayId()
                if d:
                    self._Iren.GetRenderWindow().SetDisplayId(d)

                # do the actual remap with the new parent information
                self._Iren.GetRenderWindow().WindowRemap()

                # store the new situation
                self.__handle = self.GetHandle()
                self._Iren.GetRenderWindow().Render()

    def SetRenderWhenDisabled(self, newValue):
        """Change value of __RenderWhenDisabled ivar.

        If __RenderWhenDisabled is false (the default), this widget will not
        call Render() on the RenderWindow if the top level frame (i.e. the
        containing frame) has been disabled.

        This prevents recursive rendering during wx.SafeYield() calls.
        wx.SafeYield() can be called during the ProgressMethod() callback of
        a VTK object to have progress bars and other GUI elements updated -
        it does this by disabling all windows (disallowing user-input to
        prevent re-entrancy of code) and then handling all outstanding
        GUI events.

        However, this often triggers an OnPaint() method for wxVTKRWIs,
        resulting in a Render(), resulting in Update() being called whilst
        still in progress.
        """
        self.__RenderWhenDisabled = bool(newValue)

    def _set_key_event_informations(self, event):
        ctrl, shift = event.ControlDown(), event.ShiftDown()
        alt = event.AltDown()
        keycode = event.GetKeyCode()

        key = chr(keycode if keycode<256 else 0)
        # Get keysym from the table, default to the key representation, or None
        # if the key is not <256
        keysym = self.keysyms.get(keycode, key)

        # wxPython 2.6.0.1 does not return a valid event.Get{X,Y}()
        # for this event, so we use the cached position.
        (x,y)= self._Iren.GetEventPosition()
        self._Iren.SetEventInformation(x, y,
                                       ctrl, shift, key, 0,
                                       keysym)
        if hasattr(self._Iren, "SetAltKey") :
            # SetAltKey does not exist in VTK 5.0
            self._Iren.SetAltKey(alt)

#--------------------------------------------------------------------
def wxVTKRenderWindowInteractorConeExample():
    """Like it says, just a simple example
    """
    # every wx app needs an app
    app = wx.PySimpleApp()

    # create the top-level frame, sizer and wxVTKRWI
    frame = wx.Frame(None, -1, "wxVTKRenderWindowInteractor", size=(400,400))
    widget = wxVTKRenderWindowInteractor(frame, -1)
    sizer = wx.BoxSizer(wx.VERTICAL)
    sizer.Add(widget, 1, wx.EXPAND)
    frame.SetSizer(sizer)
    frame.Layout()

    # It would be more correct (API-wise) to call widget.Initialize() and
    # widget.Start() here, but Initialize() calls RenderWindow.Render().
    # That Render() call will get through before we can setup the
    # RenderWindow() to render via the wxWidgets-created context; this
    # causes flashing on some platforms and downright breaks things on
    # other platforms.  Instead, we call widget.Enable().  This means
    # that the RWI::Initialized ivar is not set, but in THIS SPECIFIC CASE,
    # that doesn't matter.
    widget.Enable(1)

    widget.AddObserver("ExitEvent", lambda o,e,f=frame: f.Close())

    ren = vtk.vtkRenderer()
    widget.GetRenderWindow().AddRenderer(ren)

    cone = vtk.vtkConeSource()
    cone.SetResolution(8)

    coneMapper = vtk.vtkPolyDataMapper()
    coneMapper.SetInput(cone.GetOutput())

    coneActor = vtk.vtkActor()
    coneActor.SetMapper(coneMapper)

    ren.AddActor(coneActor)

    # show the window
    frame.Show()

    app.MainLoop()

if __name__ == "__main__":
    wxVTKRenderWindowInteractorConeExample()
