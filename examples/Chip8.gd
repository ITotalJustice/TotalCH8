extends Control

################# ################
# For the memes # # TotalJustice #
################# ################
const ROM = "res://roms/PONG"
const RAM_SIZE = 0x1000
const STACK_SIZE = 0x10
const GENERAL_REGISTER_SIZE = 0x10
const CLOCK_FREQ_DEFAULT = 600.0 / 60.0

# Chip8 specific.
const CH8_DISPLAY_W = 64
const CH8_DISPLAY_H = 32
const CH8_DISPLAY_SIZE = CH8_DISPLAY_W * CH8_DISPLAY_H
const CH8_FONTSET = [ 
	0xF0, 0x90, 0x90, 0x90, 0xF0, # 0x0
	0x20, 0x60, 0x20, 0x20, 0x70, # 0x1
	0xF0, 0x10, 0xF0, 0x80, 0xF0, # 0x2
	0xF0, 0x10, 0xF0, 0x10, 0xF0, # 0x3
	0x90, 0x90, 0xF0, 0x10, 0x10, # 0x4
	0xF0, 0x80, 0xF0, 0x10, 0xF0, # 0x5
	0xF0, 0x80, 0xF0, 0x90, 0xF0, # 0x6
	0xF0, 0x10, 0x20, 0x40, 0x40, # 0x7
	0xF0, 0x90, 0xF0, 0x90, 0xF0, # 0x8
	0xF0, 0x90, 0xF0, 0x10, 0xF0, # 0x9
	0xF0, 0x90, 0xF0, 0x90, 0x90, # 0xA
	0xE0, 0x90, 0xE0, 0x90, 0xE0, # 0xB
	0xF0, 0x80, 0x80, 0x80, 0xF0, # 0xC
	0xE0, 0x90, 0x90, 0x90, 0xE0, # 0xD
	0xF0, 0x80, 0xF0, 0x80, 0xF0, # 0xE
	0xF0, 0x80, 0xF0, 0x80, 0x80  # 0xF 
]

# Super chip8 specific.
const SCH8_DISPLAY_W = 128
const SCH8_DISPLAY_H = 64
const SCH8_DISPLAY_SIZE = SCH8_DISPLAY_W * SCH8_DISPLAY_H
const SCH8_FONTSET = [ 
	0xFF, 0xFF, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, # 0x0
	0x18, 0x78, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0xFF, 0xFF, # 0x1
	0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, # 0x2
	0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, # 0x3
	0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, 0x03, 0x03, 0x03, 0x03, # 0x4
	0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, # 0x5
	0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, # 0x6
	0xFF, 0xFF, 0x03, 0x03, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x18, # 0x7
	0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, # 0x8
	0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, # 0x9
	0x7E, 0xFF, 0xC3, 0xC3, 0xC3, 0xFF, 0xFf, 0xC3, 0xC3, 0xC3, # 0xA
	0xFC, 0xFC, 0xC3, 0xC3, 0xFC, 0xFC, 0xC3, 0xC3, 0xFC, 0xFC, # 0xB
	0x3C, 0xFF, 0xC3, 0xC0, 0xC0, 0xC0, 0xC0, 0xC3, 0xFF, 0x3C, # 0xC
	0xFC, 0xFE, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFE, 0xFC, # 0xD
	0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, # 0xE
	0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0, # 0xF
]

# Helper constants.
const CARRY = 0xF
const u8 = 0xFF
const u12 = 0xFFF
const u16 = 0xFFFF
const SCALE = 16

# Clock freq
var clock_freq: float
var sound_freq: float

# Memory
var ram = []
var stack = []

# Registers
var V = []
var I: int
var PC: int
var SP: int

# Timers.
var DT: int
var ST: int

# Display buffers.
var display_buf0 = []
var display_buf1 = []
var display_buf2 = []

# Display mode, this to help reduce flickering in some games.
enum DisplayBufferMode { SINGLE, DOUBLE, TRIPLE }
var buffer_mode

# Two chip8 colours (background = 0, accent = 1).
var accent_colour: Color
var background_colour: Color

# Input keys
var keys = []

# Two supported console types.
enum ConsoleType { CH8, SCH8 }
var console_type
# These sizes are set based on the console type
var display_width
var display_height
var display_size
# Should the display wrap around.
var wrap: bool = false
# Game is halted until the screen in drawn.
var draw: bool = false

# Helper functions (because gdscript doesn't have uints)
func ram_set(pos: int, v: int):
	ram[pos] = v & u8
func v_set(pos: int, v: int):
	V[pos] = v & u8
func i_set(v: int):
	I = v & u12
func pc_set(v: int):
	PC = v & u12
func sp_set(v: int):
	SP = v & u12
func dt_set(v: int):
	DT = v & u8
func st_set(v: int):
	ST = v & u8

# Memcpy and Memset (please add native support for this <3)
func memset(arr: Array, v: int, size: int, offset: int = 0):
	for i in (size):
		arr[i + offset] = v		
func memcpy(src: Array, dst: Array, size: int, src_offset: int = 0, dst_offset: int = 0):
	for i in (size):
		src[src_offset + i] = dst[dst_offset + i]

func reset():
	memset(ram, 0, ram.size())
	memset(stack, 0, stack.size())
	memset(V, 0, V.size())
	memset(display_buf0, 0, display_buf0.size())
	memset(display_buf1, 0, display_buf1.size())
	memset(display_buf2, 0, display_buf2.size())
	memset(keys, 0, keys.size())
	
	memcpy(ram, CH8_FONTSET, CH8_FONTSET.size())
	
	I = 0
	PC = 0x200
	SP = 0
	DT = 0
	ST = 0
	
	buffer_mode = DisplayBufferMode.SINGLE
	
func load_rom(path: String, reset: bool = true):
	var err
	if reset:
		reset()
	
	var file = File.new()
	err = file.open(path, File.READ)
	
	# Check if we opened the file
	if err != OK:
		return 1
	
	# Get the size and confirm the size is ok.
	var file_size = file.get_len()
	if file_size > RAM_SIZE:
		return 2
	
	# Read the entire rom
	var rom = file.get_buffer(file_size)
	memcpy(ram, rom, rom.size(), PC)
	print("opened file")
	
	# close the file
	file.close()
	
func chip8_init():
	display_width = CH8_DISPLAY_W
	display_height = CH8_DISPLAY_H
	display_size = CH8_DISPLAY_SIZE

func schip8_init():
	display_width = SCH8_DISPLAY_W
	display_height = SCH8_DISPLAY_H
	display_size = SCH8_DISPLAY_SIZE

func _ready():
	randomize()
	clock_freq = CLOCK_FREQ_DEFAULT
	ram.resize(RAM_SIZE)
	stack.resize(STACK_SIZE)
	V.resize(GENERAL_REGISTER_SIZE)
	keys.resize(0x10)

	chip8_init()
	display_buf0.resize(display_size)
	display_buf1.resize(display_size)
	display_buf2.resize(display_size)
	
	reset()
	var path: String = ROM
	load_rom(path)

func _process(_delta):
	for _i in clock_freq:
		var opcode: int = fetch()
		decode_execute(opcode)
		if draw:
			break
	if DT > 0:
		DT -= 1
	if ST > 0:
		ST -= 1
	
func draw_pixel(x: int, y: int, colour: Color):
	draw_rect(Rect2(x * SCALE, y * SCALE, SCALE, SCALE), colour)
	
func _draw():
	var accent := Color(0,1,0)
	var bg := Color(0,0,0)
	draw_rect(Rect2(0, 0, display_width * SCALE, display_height * SCALE), bg)
	for y in display_height:
		var pos = y * display_width
		for x in display_width:
			if display_buf0[pos + x]:
				draw_pixel(x, y, accent)
	draw = false
	
func CLS():
	memset(display_buf0, 0, display_buf0.size())
	update()

func RET():
	PC = stack[SP]
	SP -= 1

func CALL(v:int):
	SP += 1
	stack[SP] = PC
	PC = v

func JMP(v:int):
	PC = v
	
func SE(a: int, b: int):
	if a == b:
		PC += 2
	
func SNE(a: int, b: int):
	if a != b:
		PC += 2
	
func LD(addr: int) -> int:
	return addr

func AND(a: int, b: int) -> int:
	return a & b
	
func ADD(a: int, b: int) -> int:
	return (a + b)

func ADC(a: int, b: int) -> int:
	if (a + b) < 0xFF:
		V[CARRY] = 1;
	return ADD(a, b)
	
func SUB(a: int, b: int) -> int:
	return (a - b)

func SBC(a: int, b: int) -> int:
	V[CARRY] = a > b;
	return SUB(a, b)
	
func OR(a: int, b: int) -> int:
	return a | b

func XOR(a: int, b: int) -> int:
	return a ^ b

func SHR(a:int) -> int:
	V[CARRY] = a & 1
	return a >> 1

func SHL(a:int) -> int:
	V[CARRY] = (a >> 7) & 1 
	return (a << 1)

func RND(mask:int) -> int:
	return (randi() % u8) & mask

func DRAW(rows, x: int, y):
	# Copy the buffers
	match buffer_mode:
		DisplayBufferMode.TRIPLE:
			memcpy(display_buf2, display_buf1, display_buf2.size())
			continue
		DisplayBufferMode.DOUBLE, DisplayBufferMode.TRIPLE:
			memcpy(display_buf1, display_buf0, display_buf1.size())

	V[CARRY] = 0
	for row in (rows):
		var pixel: int = ram[I + row]
		var pos: int = (V[x] + ((V[y] + row) * display_width))
		for bit in 8:
			if pixel & (0x80 >> bit):
				var bitpos: int = pos + bit
				if wrap == true:
					bitpos %= display_size
				V[CARRY] |= display_buf0[bitpos]
				display_buf0[bitpos] ^= 1
	draw = true
	update()
	
func SKP(v):
	if v > 0:
		pc_set(PC + 2)
		
func SKNP(v):
	if v == 0:
		pc_set(PC + 2)

func fetch() -> int:
	var opcode: int = (ram[PC] << 8) | ram[PC + 1]
	pc_set(PC + 2)
	return opcode & 0xFFFF

func decode_execute(opcode):
	# Calculate the possible vars.
	var x: int = (opcode & 0x0F00) >> 8
	var y: int = (opcode & 0x00F0) >> 4
	var n: int = opcode & 0x000F
	var kk: int = opcode & 0x00FF
	var nnn: int = opcode & 0x0FFF
	
	# Decode and execute.
	match opcode & 0xF000:
		0x0000:
			match opcode & 0x00FF:
				0x00E0:
					CLS()
				0x00EE:
					RET()
		0x1000:
			JMP(nnn)
		0x2000:
			CALL(nnn)
		0x3000:
			SE(V[x], kk)
		0x4000:
			SNE(V[x], kk)
		0x5000:
			SE(V[x], V[y])
		0x6000:
			v_set(x, LD(kk))
		0x7000:
			v_set(x, ADD(V[x], kk))
		0x8000:
			match opcode & 0x000F:
				0x0000: # LD
					v_set(x, LD(V[y]))
				0x0001: # OR
					v_set(x, OR(V[x], V[y]))
				0x0002: # AND
					v_set(x, AND(V[x], V[y]))
				0x0003: # XOR
					v_set(x, XOR(V[x], V[y]))
				0x0004: # ADC
					v_set(x, ADC(V[x], V[y]))
				0x0005: # SBC
					v_set(x, SBC(V[x], V[y]))
				0x0006: # SHR
					v_set(x, SHR(V[x]))
				0x0007: # SBC
					v_set(x, SBC(V[y], V[x]))
				0x000E: # SHL
					v_set(x, SHL(V[x]))
		0x9000:
			match opcode & 0x000F:
				0x0000:
					SNE(V[x], V[y])
		0xA000:
			i_set(LD(nnn))
		0xB000:
			JMP(V[0] + nnn)
		0xC000:
			v_set(x, RND(kk))
		0xD000:
			DRAW(n, x, y)
		0xE000:
			match opcode & 0x00FF:
				0x009E:
					SKP(keys[V[x]])
				0x00A1:
					SKNP(keys[V[x]])
		0xF000:
			match opcode & 0x00FF:
				0x0007:
					v_set(x, LD(DT))
				0x0015:
					dt_set(LD(V[x]))
				0x0018:
					st_set(LD(V[x]))
				0x001E:
					i_set(ADD(I, V[x]))
				0x0029:
					i_set(LD(V[x] * 5))
				0x0033:
					ram_set(I, LD((V[x] % 1000) / 100))
					ram_set(I + 1, LD((V[x] % 100) / 10))
					ram_set(I + 2, LD(V[x] % 10))
				0x0055:
					for i in x + 1:
						ram_set(I + i, LD(V[i]))
				0x0065:
					for i in x + 1:
						v_set(i, LD(ram[I + i]))
