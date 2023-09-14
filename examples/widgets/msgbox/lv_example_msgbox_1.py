def event_cb(e):
    mbox = e.get_target_obj()
    print("Button %s clicked" % mbox.get_active_button_text())

buttons = ["Apply", "Close", ""]

mbox1 = lv.msgbox(lv.scr_act(), "Hello", "This is a message box with two buttons.", buttons, True)
mbox1.add_event(event_cb, lv.EVENT.VALUE_CHANGED, None)
mbox1.center()

