#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

.SECONDARY:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	$(shell basename $(CURDIR))
BUILD		:=	build
SOURCES 	:= 	$(shell [ -d source ] && find source -type d) # source folder + all directories inside it
DATA		:=
INCLUDES	:=	include
GRAPHICS	:= 	data
AUDIO       := 	audio
PRECOMPILED := 	precompiled

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-mthumb-interwork

CFLAGS	:=	-g -Wall -O0\
 		-march=armv5te -mtune=arm946e-s -fomit-frame-pointer\
		-ffast-math \
		$(ARCH)

CFLAGS	+=	$(INCLUDE) -DARM9
CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g $(ARCH)
LDFLAGS	=	-specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS	:= -lfat -lmm9 -ldswifi9 -lnds9 -lm


#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=	$(LIBNDS)

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT 		:= $(CURDIR)/$(BUILD)/$(TARGET)
export OUTPUT_NDS 	:= $(CURDIR)/$(TARGET)

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
					$(foreach dir,$(GRAPHICS),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES 	:=  $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*))) $(if $(wildcard $(AUDIO)/*.*),soundbank.bin)
PNGFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.png)))
GRITFILES	:=	$(foreach dir,$(GRAPHICS),$(notdir $(wildcard $(dir)/*.grit)))
PRECFILES	:=	$(foreach dir,$(PRECOMPILED),$(notdir $(wildcard $(dir)/*.o)))

# Keep only PNG files that have a grit file
BASES_PNG := $(basename $(PNGFILES))
BASES_GRIT := $(basename $(GRITFILES))
PNGFILES := $(foreach base,$(BASES_PNG),$(if $(filter $(base),$(BASES_GRIT)),$(base).png))

# build audio file list, include full path
export AUDIOFILES	:=	$(foreach dir,$(notdir $(wildcard $(AUDIO)/*.*)),$(CURDIR)/$(AUDIO)/$(dir))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
					$(PNGFILES:.png=.o) \
					$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)\
					$(PRECFILES)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).nds $(TARGET).ds.gba


#---------------------------------------------------------------------------------
#---------------------------------------------------------------------------------
else

DEPENDS := $(OFILES:.o=.d) precompiled

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
# $(LD) $(LDFLAGS) $(OFILES) $(LIBPATHS) $(LIBS) -o $(OUTPUT).nds
# @mv $(OUTPUT).nds $(OUTPUT_NDS).nds

$(OUTPUT).nds 	: $(OUTPUT).elf

# Rules to compile .cpp and .c files (we need g++ for mixed c/c++ projects)
%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) $< -o $@

%.o: %.c
	$(LD) -c $(CFLAGS) $< -o $@

$(OUTPUT).elf : $(OFILES)
	@echo "Linking..."
	$(LD) $(LDFLAGS) -Wl,-Map,$(OUTPUT).map $(OFILES) $(LIBPATHS) $(LIBS) -o $@
	@echo "Creating disassembly..."
	@arm-none-eabi-objdump -D $(OUTPUT).elf > $(OUTPUT).s

# Adjust the clean rule:
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).nds $(OUTPUT).elf $(OUTPUT).s

#---------------------------------------------------------------------------------
%.bin.o : %.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
# Copying precompiled code
#---------------------------------------------------------------------------------
precompiled:
	@ if [ -d $(CURDIR)/../$(PRECOMPILED) ] && [ "$(wildcard $(CURDIR)/../$(PRECOMPILED)/*)" != "" ]; then \
    	cp $(CURDIR)/../$(PRECOMPILED)/* .; \
  	fi

#---------------------------------------------------------------------------------
# rule to build soundbank from music files
#---------------------------------------------------------------------------------
soundbank.bin : $(AUDIOFILES)
#---------------------------------------------------------------------------------
	@mmutil $^ -osoundbank.bin -hsoundbank.h -d

#---------------------------------------------------------------------------------
%.s %.h : %.png %.grit
#---------------------------------------------------------------------------------
	grit $< -fts -o$*
	@if [ "$*" = "ds_menu" ]; then \
		sed -i '' 's/\.hword 0x7C1F/.hword 0x7BBD/' $*.s; \
	fi

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
