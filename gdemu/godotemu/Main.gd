extends Node

@onready var emulator = $Emulator
@onready var drive1_access = $Control/Drive1Access
@onready var drive2_access = $Control/Drive2Access

@export var is_disk_encrypt: bool = true
@export var drivea_path: String
@export var driveb_path: String
@export var harddisk0_path: String

@export var rom_path: String

var popup: Window

const border_width = 16
const border_height = 16

# x1
var screen_width = 640.0
var screen_height = 400.0
var window_width_aspect = 640.0
var sample_hz: float = 62500.0

##msx
#var screen_width = 512.0
#var screen_height = 384.0
#var window_width_aspect = 576.0
#var sample_hz: float = 48000.0

## pc8801ma
#var screen_width = 640.0
#var screen_height = 400.0
#var window_width_aspect = 640.0
#var sample_hz: float = 55467.0


var sound_samples: int = 0;
var sound_play_offset: int = 0;

var playback: AudioStreamPlayback = null

var screenImage: Image = Image.new()
var screenImageTexture: ImageTexture = ImageTexture.new()

var config: ConfigFile = ConfigFile.new()

var QuitDialog = preload("res://QuitDialog.tscn")

func load_config() -> void:
	var _result = config.load("user://config.dat")
	
func save_config() -> void:
	config.set_value("screen", "is_fullscreen", ((get_window().mode == Window.MODE_EXCLUSIVE_FULLSCREEN) or (get_window().mode == Window.MODE_FULLSCREEN)))
	var _result = config.save("user://config.dat")

func _fill_buffer():
	var to_fill = playback.get_frames_available()
	if sound_samples == 0:
		while to_fill > 0:
			playback.push_frame(Vector2(0, 0))
			to_fill -= 1
		return

	var buff: PackedVector2Array = emulator.get_sound_buffer()
	while to_fill > 0:
		sound_play_offset += 1
		sound_play_offset %= (sound_samples * 2 )
		emulator.set_play_point(sound_play_offset)
		playback.push_frame(buff[sound_play_offset]) # Audio frames are stereo.
#		playback.push_frame(Vector2(randi(), randi()))
		to_fill -= 1

func _ready() -> void:
	# ウィンドウサイズを設定する(現状常に初期状態に戻る)
#	get_tree().set_screen_stretch(SceneTree.stretch_mode.STRETCH_MODE_2D,SceneTree.STRETCH_ASPECT_KEEP, Vector2(window_width_aspect + 16,screen_height + 32), 1)
	var viewport_width = window_width_aspect + border_width
	var viewport_height = screen_height + border_height + 16

	get_tree().root.content_scale_size = Vector2(viewport_width, viewport_height)
	get_tree().root.size = Vector2(viewport_width, viewport_height)
	get_tree().root.content_scale_mode = Window.CONTENT_SCALE_MODE_CANVAS_ITEMS
	emulator.initialize()

	load_config()
	var is_fullscreen = config.get_value("screen", "is_fullscreen", false)
	get_window().mode = Window.MODE_EXCLUSIVE_FULLSCREEN if (is_fullscreen) else Window.MODE_WINDOWED

	Engine.max_fps = 0

	screenImage = Image.create(screen_width + border_width, screen_height + border_width, false, Image.FORMAT_RGB8)
	screenImageTexture = ImageTexture.create_from_image(screenImage)
#	screenImageTexture.set_image(screenImage)
#	eset_size_override(Vector2(screen_width, screen_height))
	# screenImageTexture.create(screen_width, screen_height, Image.FORMAT_RGB8)
	emulator.texture = screenImageTexture

	$AudioPlayer.stream.mix_rate = sample_hz # Setting mix rate is only possible before play().
	$AudioPlayer.play()
	playback = $AudioPlayer.get_stream_playback()
	_fill_buffer()
	# $AudioPlayer.play()


	emulator.setup()
	emulator.set_disk_encrypt(is_disk_encrypt)

	sound_samples = emulator.get_sound_samples()
	sound_play_offset = 0

	# for X1
	# res://の中のディスクイメージを開く(user dirにコピーしてから使われる)
	if !drivea_path.is_empty():
		emulator.open_floppy_disk(0, drivea_path, 0);
	if !driveb_path.is_empty():
		emulator.open_floppy_disk(1, driveb_path, 0);
	
	if !harddisk0_path.is_empty():
		emulator.open_hard_disk(0, harddisk0_path)

#	# for MSX
#	emulator.open_cart(0,rom_path)

#	# for PC8801MA
#	if !drivea_path.is_empty():
#		emulator.open_floppy_disk(0, drivea_path, 0);
#	if !driveb_path.is_empty():
#		emulator.open_floppy_disk(1, driveb_path, 0);

	emulator.scale.x = window_width_aspect / screen_width

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(_delta: float) -> void:
	var data = emulator.get_data()
	# screenImage.data.data = data
	screenImage.set_data(screen_width + border_width, screen_height + border_width, false, Image.FORMAT_RGB8, data)
	screenImageTexture.update(screenImage)
	emulator.texture = screenImageTexture
	_fill_buffer()
	# なぜか再生が止まり、get_frames_available()が0を返すようになるので、
	# 止まった場合再取得するよう強引に対応
	if not $AudioPlayer.playing:
		$AudioPlayer.play()
		playback = $AudioPlayer.get_stream_playback()

	# update joystick 1 state
	var state = ( ( 1 if Input.is_action_pressed("joystick_up")   else 0) 
				| ( 2 if Input.is_action_pressed("joystick_down") else 0)
				| ( 4 if Input.is_action_pressed("joystick_left") else 0)
				| ( 8 if Input.is_action_pressed("joystick_right") else 0)
				| (16 if Input.is_action_pressed("joystick_a") else 0)
				| (32 if Input.is_action_pressed("joystick_b") else 0)
				)
	emulator.update_joystate(0, state)
	
	var access_flag = emulator.is_floppy_disk_accessed()
	drive1_access.visible = access_flag & 0x01
	drive2_access.visible = access_flag & 0x02


func show_quitdialog() -> void:
	# ポップアップウィンドウ呼び出し
	if popup == null:
		popup = QuitDialog.instantiate()

		# popup終了時に "_resume()" を呼び出す
		popup.connect('back_to_application',Callable(self,'_resume'))
		popup.connect('popup_hide', Callable(self,'_resume'))
		# ポーズ開始。ゲームを止める
		popup.popup_exclusive_centered(self)
	else:
		popup.show()

	get_tree().paused = true

func _resume():
	# ウィンドウを閉じたときのコールバック
	var tree = get_tree()
	if tree:
		# ポーズ解除
		tree.paused = false 

func _input(event):
	if event.is_action_pressed("exit_application"):
		show_quitdialog()
	if event.is_action_pressed("toggle_fullscreen"):
		get_window().mode = Window.MODE_EXCLUSIVE_FULLSCREEN if (!((get_window().mode == Window.MODE_EXCLUSIVE_FULLSCREEN) or (get_window().mode == Window.MODE_FULLSCREEN))) else Window.MODE_WINDOWED
		save_config()
		# もう一回ALTを押してromaji to kanaを解除しておく……(強引)
		emulator.key_down(KEY_ALT, true)

	if event is InputEventKey:
		if event.pressed:
			emulator.key_down(event.keycode, event.echo)
		else:
			emulator.key_up(event.keycode, event.echo)

func _notification(what):
	if what == NOTIFICATION_WM_CLOSE_REQUEST:
		get_tree().quit()

