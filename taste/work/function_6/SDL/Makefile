#Taste::Instances_Max 1
#Taste::Instances_Min 1
#Taste::Startup_Priority 1
#TASTE_IV_Properties::Default_Codegen Ada
SEDS_CONVERTER?=spacecreator.AppImage --sedsconverter
OPENGEODE?=opengeode
VISUAL?=kate

all: compile-linux

clean:
	rm -rf obj code wrappers *.gpr src/*.md5 src/*.asn src/*.autosave src/Makefile* src/*.pr.ini src/*.gpr src/system_structure.pr

edit:
	cd src && $(OPENGEODE) --edit --toAda --taste system_structure.pr function_6.pr

sdl-to-if:
	cd src &&\
	   sdl2if --output-dir ../../../build/modelchecking  ../src/function_6.pr ../src/system_structure.pr

# Ada
generate-code:
	rm -rf code/*.[hc] && mkdir -p code && cd code && \
	   $(OPENGEODE) --taste  --toAda ../src/function_6.pr ../src/system_structure.pr && \
	   mv *.gpr ..

compile-linux: generate-code
	   CODE_PATH=code ADA_INCLUDE_PATH=../../../dataview/Ada gprbuild -p -P function_6_ada.gpr

