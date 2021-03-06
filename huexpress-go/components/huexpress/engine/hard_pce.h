#ifndef _INCLUDE_HARD_PCE_H
#define _INCLUDE_HARD_PCE_H

#include "../config.h"
#include "stddef.h"
#include "utils.h"

// System clocks (hz)
#define CLOCK_MASTER           (21477270)
#define CLOCK_TIMER            (CLOCK_MASTER / 3)
#define CLOCK_CPU              (CLOCK_MASTER / 3)
#define CLOCK_PSG              (CLOCK_MASTER / 6)

// Timings (we don't support CSH/CSL yet...)
#define CYCLES_PER_FRAME       (CLOCK_CPU / 60)
#define CYCLES_PER_LINE        (CYCLES_PER_FRAME / 263 + 1)
#define CYCLES_PER_TIMER_TICK  (1024) // 1097

// VDC Status Flags (vdc_status bit)
typedef enum {
	VDC_STAT_CR  = 0,	/* Sprite Collision */
	VDC_STAT_OR  = 1,	/* Sprite Overflow */
	VDC_STAT_RR  = 2,	/* Scanline interrupt */
	VDC_STAT_DS  = 3,	/* End of VRAM to SATB DMA transfer */
	VDC_STAT_DV  = 4,	/* End of VRAM to VRAM DMA transfer */
	VDC_STAT_VD  = 5,	/* VBlank */
	VDC_STAT_BSY = 6,	/* DMA Transfer in progress */
} vdc_stat_t;

// VDC Registers
typedef enum {
	MAWR 	= 0,		/* Memory Address Write Register */
	MARR 	= 1,		/* Memory Address Read Register */
	VRR 	= 2,		/* VRAM Read Register */
	VWR 	= 2,		/* VRAM Write Register */
	vdc3 	= 3,		/* Unused */
	vdc4 	= 4,		/* Unused */
	CR 		= 5,		/* Control Register */
	RCR 	= 6,		/* Raster Compare Register */
	BXR 	= 7,		/* Horizontal scroll offset */
	BYR 	= 8,		/* Vertical scroll offset */
	MWR 	= 9,		/* Memory Width Register */
	HSR 	= 10,		/* Unknown, other horizontal definition */
	HDR 	= 11,		/* Horizontal Definition */
	VPR 	= 12,		/* Higher byte = VDS, lower byte = VSW */
	VDW 	= 13,		/* Vertical Definition */
	VCR 	= 14,		/* Vertical counter between restarting of display */
	DCR 	= 15,		/* DMA Control */
	SOUR 	= 16,		/* Source Address of DMA transfert */
	DISTR 	= 17,		/* Destination Address of DMA transfert */
	LENR 	= 18,		/* Length of DMA transfert */
	SATB 	= 19		/* Address of SATB */
} vdc_reg_t;

#define PSG_VOICE_REG           0x00 // voice index
#define PSG_VOLUME_REG          0x01 // master volume
#define PSG_FREQ_LSB_REG        0x02 // lower 8 bits of 12 bit frequency
#define PSG_FREQ_MSB_REG        0x03 // actually most significant nibble
#define PSG_DDA_REG             0x04
#define PSG_BALANCE_REG         0x05
#define PSG_DATA_INDEX_REG      0x06
#define PSG_NOISE_REG           0x07

#define PSG_DDA_ENABLE          0x80 // bit 7
#define PSG_DDA_DIRECT_ACCESS   0x40 // bit 6
#define PSG_DDA_VOICE_VOLUME    0x1F // bits 0-4
#define PSG_BALANCE_LEFT        0xF0 // bits 4-7
#define PSG_BALANCE_RIGHT       0x0F // bits 0-3
#define PSG_NOISE_ENABLE        0x80 // bit 7

#define PSG_DA_BUFSIZE          1024
#define PSG_CHANNELS            6


typedef union {
	struct {
		uint8_t l, h;
	} B;
	uint16_t W;
} UWord;


/* The structure containing all variables relatives to Input and Output */
typedef struct {
	/* VCE */
	UWord VCE[0x200];			/* palette info */
	UWord vce_reg;				/* currently selected color */

	/* VDC */
	UWord VDC[32];				/* value of each VDC register */
	uint8_t vdc_reg;			/* currently selected VDC register */
	uint8_t vdc_status;			/* current VCD status (end of line, end of screen, ...) */
	uint8_t vdc_satb;			/* DMA transfer status to happen in vblank */
	uint8_t vdc_mode_chg;       /* Video mode change needed at next frame */
	uint32_t vdc_irq_queue;		/* Pending VDC IRQs (we use it as a stack of 4bit events) */

	/* joypad */
	uint8_t JOY[8];				/* value of pressed button/direct for each pad */
	uint8_t joy_select;			/* used to know what nibble we must return */
	uint8_t joy_counter;		/* current addressed joypad */

	/* PSG */
	uint8_t PSG[PSG_CHANNELS][8];
	uint8_t PSG_WAVE[PSG_CHANNELS][32];
	// PSG STRUCTURE
	// 0 : dda_out
	// 2 : freq (lo byte)  | In reality it's a divisor
	// 3 : freq (hi byte)  | 3.7 Mhz / freq => true snd freq
	// 4 : dda_ctrl
	//     000XXXXX
	//     ^^  ^
	//     ||  ch. volume
	//     ||
	//     |direct access (everything at byte 0)
	//     |
	//    enable
	// 5 : pan (left vol = hi nibble, right vol = low nibble)
	// 6 : wave ringbuffer index
	// 7 : noise data for channels 5 and 6

	uint8_t psg_ch, psg_volume, psg_lfo_freq, psg_lfo_ctrl;

	uint8_t psg_da_data[PSG_CHANNELS][PSG_DA_BUFSIZE];
	uint16_t psg_da_index[PSG_CHANNELS];
	uint16_t psg_da_count[PSG_CHANNELS];

	/* TIMER */
	uint8_t timer_running, timer_reload, timer_counter;
	uint32_t timer_cycles_counter;

	/* IRQ */
	uint8_t irq_mask, irq_status;

	/* Remanence latch */
	uint8_t io_buffer;

} IO_t;


typedef struct {
	// Main memory
	uint8_t RAM[0x2000];

	// Extra RAM contained on the HuCard (Populous)
	uint8_t *ExtraRAM;

	// Backup RAM
	uint8_t BackupRAM[0x800];

	// Video RAM
	uint8_t VRAM[0x10000];

	// Sprite RAM
	// The pc engine got a function to transfert a piece VRAM toward the inner
	// gfx cpu sprite memory from where data will be grabbed to render sprites
	uint16_t SPRAM[64 * 4];

	// ROM memory
	uint8_t *ROM, *ROM_PTR;

	// ROM size in 0x2000 blocks
	uint16_t ROM_SIZE;

	// ROM crc
	uint32_t ROM_CRC;

	// NULLRAM traps read/writes to unmapped areas
	uint8_t NULLRAM[0x2004];

	// PCE->PC Palette convetion array
	// Each of the 512 available PCE colors (333 RGB -> 512 colors)
	// got a correspondance in the 256 fixed colors palette
	uint8_t Palette[512];

	// The current rendered line on screen
	uint16_t Scanline;

	// Total number of elapsed cycles in the current scanline
	int16_t Cycles;

	// Value of each of the MMR registers
	uint8_t MMR[8];

	// Street Fighter 2 Mapper
	uint8_t SF2;

	// IO Registers
	IO_t IO;

} PCE_t;


#include "h6280.h"


// The global structure for all hardware variables
extern PCE_t PCE;

// physical address on emulator machine of each of the 256 banks
extern uint8_t *PageR[8];
extern uint8_t *PageW[8];
extern uint8_t *MemoryMapR[256];
extern uint8_t *MemoryMapW[256];

#define RAM PCE.RAM
#define BackupRAM PCE.BackupRAM
#define ExtraRAM PCE.ExtraRAM
#define SPRAM PCE.SPRAM
#define VRAM PCE.VRAM
#define NULLRAM PCE.NULLRAM
#define IOAREA (NULLRAM + 4)
#define ROM PCE.ROM
#define ROM_PTR PCE.ROM_PTR
#define ROM_CRC PCE.ROM_CRC
#define ROM_SIZE PCE.ROM_SIZE
#define Scanline PCE.Scanline
#define Palette PCE.Palette
#define Cycles PCE.Cycles
#define MMR PCE.MMR
#define SF2 PCE.SF2
#define io PCE.IO

#define IO_VDC_REG           io.VDC
#define IO_VDC_REG_ACTIVE    io.VDC[io.vdc_reg]
#define IO_VDC_REG_INC(reg)  {uint8_t _i[] = {1,32,64,128}; io.VDC[(reg)].W += _i[(io.VDC[CR].W >> 11) & 3];}
#define IO_VDC_STATUS(bit)   ((io.vdc_status >> bit) & 1)
#define IO_VDC_MINLINE       (IO_VDC_REG[VPR].B.h + IO_VDC_REG[VPR].B.l)
#define IO_VDC_MAXLINE       (IO_VDC_MINLINE + IO_VDC_REG[VDW].W)

#define IO_VDC_SCREEN_WIDTH  ((IO_VDC_REG[HDR].B.l + 1) * 8)
#define IO_VDC_SCREEN_HEIGHT (IO_VDC_REG[VDW].W + 1)

// Interrupt enabled
#define SATBIntON  (IO_VDC_REG[DCR].W & 0x01)
#define DMAIntON   (IO_VDC_REG[DCR].W & 0x02)
#define SpHitON    (IO_VDC_REG[CR].W & 0x01)
#define OverON     (IO_VDC_REG[CR].W & 0x02)
#define RasHitON   (IO_VDC_REG[CR].W & 0x04)
#define VBlankON   (IO_VDC_REG[CR].W & 0x08)

#define SpriteON   (IO_VDC_REG[CR].W & 0x40)
#define ScreenON   (IO_VDC_REG[CR].W & 0x80)

#define DMA_TRANSFER_COUNTER 0x80
#define DMA_TRANSFER_PENDING 0x40

/**
 * Exported Functions
 */

int  pce_init(void);
void pce_reset(void);
void pce_term(void);
void pce_run(void);

void IO_write(uint16_t A, uint8_t V);
uint8_t IO_read(uint16_t A);


/**
 * Inlined Functions
 */

#if USE_MEM_MACROS

#define pce_read8(addr) ({							\
	uint16_t a = (addr);							\
	uint8_t *page = PageR[a >> 13]; 				\
	(page == IOAREA) ? IO_read(a) : page[a]; 	    \
})

#define pce_write8(addr, byte) {					\
	uint16_t a = (addr), b = (byte); 				\
	uint8_t *page = PageW[a >> 13]; 				\
	if (page == IOAREA) IO_write(a, b); 		    \
	else page[a] = b;							    \
}

#define pce_read16(addr) ({							\
	uint16_t a = (addr); 							\
	*((uint16_t*)(PageR[a >> 13] + a));			    \
})

#define pce_write16(addr, word) {					\
	uint16_t a = (addr), w = (word); 				\
	*((uint16_t*)(PageR[a >> 13] + a)) = w;		    \
}

#else

static inline uint8_t
pce_read8(uint16_t addr)
{
	uint8_t *page = PageR[addr >> 13];

	if (page == IOAREA)
		return IO_read(addr);
	else
		return page[addr];
}

static inline void
pce_write8(uint16_t addr, uint8_t byte)
{
	uint8_t *page = PageW[addr >> 13];

	if (page == IOAREA)
		IO_write(addr, byte);
	else
		page[addr] = byte;
}

static inline uint16_t
pce_read16(uint16_t addr)
{
	return (*((uint16_t*)(PageR[addr >> 13] + (addr))));
}

static inline void
pce_write16(uint16_t addr, uint16_t word)
{
	*((uint16_t*)(PageR[addr >> 13] + (addr))) = word;
}

#endif


static inline void
pce_bank_set(uint8_t P, uint8_t V)
{
	TRACE_IO("Bank switching (MMR[%d] = %d)\n", P, V);

	MMR[P] = V;
	PageR[P] = (MemoryMapR[V] == IOAREA) ? (IOAREA) : (MemoryMapR[V] - P * 0x2000);
	PageW[P] = (MemoryMapW[V] == IOAREA) ? (IOAREA) : (MemoryMapW[V] - P * 0x2000);
}

#endif
