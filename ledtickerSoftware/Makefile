all: bin/vefxio.dll bin/ledticker_standalone.exe bin/ledticker_config.exe flash/ledticker_flash.exe

LD_FLAGS=-static -s -O3
LD_FLAGS_SHARED=-s -O3

obj/%.o: src/%.c
	@mkdir -p obj
	gcc -c $< -o $@ -Ibemanitools-sdk
obj/%.o: src/%.rc
	@mkdir -p obj
	windres $< $@
bin/vefxio.dll: obj/vefxio.o obj/general.o
	@mkdir -p bin
	gcc $^ -o $@ -shared $(LD_FLAGS_SHARED) -lWinmm
bin/ledticker_config.exe: obj/general.o obj/config_tool.o obj/config_res.o obj/ledticker_config.o obj/config_manifest.o
	@mkdir -p bin
	gcc $^ -o $@ -mwindows $(LD_FLAGS) -lcomctl32
bin/ledticker_standalone.exe: obj/general.o obj/ledticker_standalone.o obj/standalone_res.o obj/gamelist.o obj/config_tool.o obj/config_res.o
	@mkdir -p bin
	gcc $^ -o $@ $(LD_FLAGS) -lpsapi -lcomctl32
	
flash/ledticker_flash.exe: obj/general.o obj/ledticker_flash.o obj/flash_res.o obj/config_tool.o obj/config_res.o
	@mkdir -p bin
	gcc $^ -o $@ $(LD_FLAGS) -lcomctl32
	
bin/ledticker_standalone32.exe: obj/general.o obj/ledticker_standalone.o obj/standalone_res.o obj/gamelist.o obj/config_tool.o obj/config_res.o
	@mkdir -p bin
	gcc $^ -o $@ $(LD_FLAGS) -lpsapi -lcomctl32 -m32
	


.PHONY: clean run_config run_standalone
clean:
	rm obj/*.o
run_config: bin/ledticker_config.exe
	bin/ledticker_config.exe
run_standalone: bin/ledticker_standalone.exe
	start bin/ledticker_standalone.exe
