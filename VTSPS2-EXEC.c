// A simple homebrew to demonstrate EXEC on the PS2
// VTSPS2-EXEC Written by VTSTech (veritas@vts-tech.org)
// v0.1 2020-06-11 2:21:27 AM
// First release

#include "VTSPS2-EXEC.h"

extern void loader_elf;

extern u64 WaitTime;
extern u64 CurrTime;

u64 Timer(void);

u64 WaitTime;
u64 CurrTime;
u64 init_delay_start;
u64 timeout_start;

u64 Timer(void)
{
	return (clock() / (CLOCKS_PER_SEC / 1000));
}

typedef struct {
	u8	ident[16];
	u16	type;
	u16	machine;
	u32	version;
	u32	entry;
	u32	phoff;
	u32	shoff;
	u32	flags;
	u16	ehsize;
	u16	phentsize;
	u16	phnum;
	u16	shentsize;
	u16	shnum;
	u16	shstrndx;
} elf_header_t;

typedef struct {
	u32	type;
	u32	offset;
	void	*vaddr;
	u32	paddr;
	u32	filesz;
	u32	memsz;
	u32	flags;
	u32	align;
} elf_pheader_t;

//thx sp193
void ResetIOP()
{
	SifInitRpc(0);           //Initialize SIFRPC and SIFCMD. Although seemingly unimportant, this will update the addresses on the EE, which can prevent a crash from happening around the IOP reboot.
	SifIopReset("", 0);      //Reboot IOP with default modules (empty command line)
	while(!SifIopSync()){}   //Wait for IOP to finish rebooting.
	SifInitRpc(0);           //Initialize SIFRPC and SIFCMD.
	SifLoadFileInit();       //Initialize LOADFILE RPC.
	fioInit();               //Initialize FILEIO RPC.
	sbv_patch_enable_lmb();  //SBV Patches
	sbv_patch_disable_prefix_check(); //SBV Patch Load Executable IRX And ELF Files From User-Writable Storage
}

void InitPS2()
{
	init_scr();
	ResetIOP();
	SifInitIopHeap();
	SifLoadFileInit();
	fioInit();
	//wipeUserMem();
	sbv_patch_disable_prefix_check();
	SifLoadModule("rom0:SIO2MAN", 0, NULL);
	SifLoadModule("rom0:MCMAN", 0, NULL);
	SifLoadModule("rom0:MCSERV", 0, NULL);
	SifLoadModule("rom0:PADMAN", 0, NULL);
	//sceCdInit(SCECdINoD);
	//cdInitAdd();
	sleep(1);
}

void menu(){
	scr_printf(" \n* Press X to Run TEST.ELF using MPLUS-LOADER3 Method ... \n");
	scr_printf(" \n* Press O to Run TEST.ELF using wLE LOADER.ELF Method ... \n");
	//scr_printf(" \n* Press SEL for DriveState() \n");
	//scr_printf(" \n* Press START to exit \n \n");
}

void banner(){
	scr_printf("VTSPS2-EXEC v0.2 by VTSTech (06.11.2020) \n");
	scr_printf("=======================www.vts-tech.org= \n \n");
}

//** Function LoadElf() from main.c MPLUS-LOADER3.ELF
//** http://lukasz.dk/2008/04/22/datel-memory-plus-64-mb/
//slightly modified
void LoadElf(const char *elf, char* path)
{
	char* args[1];
	t_ExecData exec;	

	SifLoadElf(elf, &exec);

	#ifdef DEBUG
		//scr_printf("Trying to load ELF: %s\n", elf);
	#endif	

	if(exec.epc > 0)
	{	
		FlushCache(0);
		FlushCache(2);

		// Reset IOP, since not all ELF's do it and we've loaded additional IOP
		// modules which need to be unloaded
		ResetIOP();

		if(path != 0)
		{
			args[0] = path;
			ExecPS2((void*)exec.epc, (void*)exec.gp, 1, args);
		}
		else
		{
			ExecPS2((void*)exec.epc, (void*)exec.gp, 0, NULL);
		}
	}
}

int main()
{
	char CWD[256];
	char ELF_PATH[256];
	InitPS2();
	setupPad();
	WaitTime = Timer();
	banner(); // Display Banner
	sleep(1);
	int state = padGetState(0,0);
	if (state == 6) { //Proceed only if Controller 1 is ready.
		menu(); // Display the Menu
		while(1){
			state = readpad(); //Constantly get button pressed.
			//SEL = 1
			//L3  = 2
			//R3  = 4
			//STR = 8
			//UP  = 16
			//RGT = 32
			//DWN = 64
			//LFT = 128
			//L2  = 256
			//R2  = 512
			//L1  = 1024
			//R1  = 2048
			// /\ = 4096
			// O  = 8192
			// X  = 16384
			//[ ] = 32768	
			if (new_pad == 16384) {
				getcwd(CWD,sizeof(CWD));
				scr_printf("CWD: %s \n",CWD);
				strcpy(ELF_PATH,CWD);
				strcat(ELF_PATH,"TEST.ELF");
				scr_printf("Launching TEST.ELF with MPLUS-LOADER3 LoadElf() ...\n");
				sleep(2);				
				LoadElf(ELF_PATH,CWD);
				scr_printf("If you can read this. It failed. \n");
			} else if (new_pad == 8192) {
				getcwd(CWD,sizeof(CWD));
				scr_printf("CWD: %s \n",CWD);
				strcpy(ELF_PATH,CWD);
				strcat(ELF_PATH,"TEST.ELF");
				scr_printf("Launching TEST.ELF with wLaunchElf's LOADER.ELF ...\n");
				//Launcher embed code from iLaunchElf
				sleep(2);
				u8 *pdata, *dest;
				elf_header_t *eh;
				elf_pheader_t *eph;
				char arg0[256], arg1[256], arg2[256], arg3[256], arg4[256], arg5[256], arg6[256], arg7[256], arg8[256];
				char *exec_args[9] = { arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8 };	
				int i,j;
				int argc = 1;
				strcpy(arg0,ELF_PATH);
				strcpy(exec_args[0],ELF_PATH);
				sleep(2);
				/* Load the embedded wLaunchELF's loader.elf to its load address, by parsing its ELF header */
				eh = (elf_header_t *)&loader_elf;
				eph = (elf_pheader_t *)(&loader_elf + eh->phoff);
				for(i = 0; i < eh->phnum; i++) {
					dest = (u8*)(eph[i].vaddr);
					pdata = (u8*)(&loader_elf + eph[i].offset);
					for(j = 0; j < eph[i].filesz; j++) dest[j] = pdata[j];
					if(eph[i].memsz > eph[i].filesz) {
						dest = (u8 *)(eph[i].vaddr + eph[i].filesz);
						for(j = 0; j < eph[i].memsz - eph[i].filesz; j++) dest[j] = '\0';
					}
				}
				padPortClose(0, 0);
				padEnd();
				SifExitRpc();
				FlushCache(0);
				FlushCache(2);
				ExecPS2((void *)eh->entry, 0, argc, exec_args);
				sleep(2);
			}
		}
	}
	sleep(10);
	return 0; //Exit normally. We should not get here.
}
