extends Popup

signal back_to_application

func _on_NoButton_pressed():
	emit_signal("back_to_application")
	queue_free()

func _on_YesButton_pressed() -> void:
	get_tree().root.propagate_notification(NOTIFICATION_WM_CLOSE_REQUEST)

