extends PopupDialog

signal back_to_application

func _on_NoButton_pressed():
	emit_signal("back_to_application")
	queue_free()

func _on_YesButton_pressed() -> void:
	get_tree().notification(MainLoop.NOTIFICATION_WM_QUIT_REQUEST)

