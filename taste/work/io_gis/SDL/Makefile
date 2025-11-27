#Taste::Instances_Max 1
#Taste::Instances_Min 1
#Taste::Startup_Priority 1
#TASTE_IV_Properties::Default_Codegen C
SEDS_CONVERTER?=spacecreator.AppImage --sedsconverter
OPENGEODE?=opengeode
VISUAL?=kate

all: compile-linux

clean:
	rm -rf obj code wrappers *.gpr src/*.md5 src/*.asn src/*.autosave src/Makefile* src/*.pr.ini src/*.gpr src/system_structure.pr

edit:
	cd src && $(OPENGEODE) --edit --toC --taste system_structure.pr io_gis.pr

sdl-to-if:
	cd src &&\
	   sdl2if --output-dir ../../../build/modelchecking  ../src/io_gis.pr ../src/system_structure.pr

# C
generate-code:
	rm -rf code/*.ad[sb] && mkdir -p code && cd code && \
	   $(OPENGEODE) --taste  --toC ../src/io_gis.pr ../src/system_structure.pr && \
	:

compile-linux: generate-code
	   CODE_PATH=code ADA_INCLUDE_PATH=../../../dataview/Ada gprbuild -p -P io_gis_ada.gpr

