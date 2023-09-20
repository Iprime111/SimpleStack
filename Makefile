srcDir = src
outDir = build
libDirs = headers CustomAssert/headers CustomAssert/ColorConsole/headers
sources = main.cpp Stack.cpp
target = Stack

ifndef buildDir_Stack

libFlags = $(addprefix -I, $(libDirs))

.PHONY: all clean debug release

all: debug

clean:
	@rm -f $(outDir)/debug/*.o all
	@rm -f $(outDir)/debug/$(target)
	@rm -f $(outDir)/release/*.o all
	@rm -f $(outDir)/release/$(target)

	cd CustomAssert && make clean

debug: export CXX = g++
debug: export CXXFLAGS = $(libFlags) -D_SHOW_STACK_TRACE -ggdb3 -std=c++17 -O0 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations -Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts -Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix -Wno-missing-field-initializers -Wno-narrowing -Wno-old-style-cast -Wno-varargs -Wstack-protector -fcheck-new -fsized-deallocation -fstack-protector -fstrict-overflow -fno-omit-frame-pointer -Wlarger-than=8192 -Wstack-usage=8192 -pie -fPIE -Werror=vla -Wno-write-strings -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr #-flto-odr-type-merging
debug: export buildDir_Stack = $(outDir)/debug
debug: export libObjects = CustomAssert/build/debug/Logger.o CustomAssert/build/debug/CustomAssert.o CustomAssert/ColorConsole/build/debug/ColorConsole.o
debug: export makeMode = debug

release: export CXX = g++
release: export CXXFLAGS = $(libFlags) -D_NDEBUG -std=c++17 -Wno-write-strings -O3
release: export buildDir_Stack = $(outDir)/release
release: export libObjects = CustomAssert/build/release/Logger.o CustomAssert/build/release/CustomAssert.o CustomAssert/ColorConsole/build/release/ColorConsole.o
release: export makeMode = release


debug release:
	@$(MAKE)

else

srcObjects = $(addprefix $(buildDir_Stack)/, $(sources:.cpp=.o))

.PHONY: all prepare build_docs

all: $(buildDir_Stack)/$(target)

# TODO -M -MD -MMD -MF ...

$(srcObjects): $(buildDir_Stack)/%.o: $(srcDir)/%.cpp
	@echo [CXX] -c $< -o $@
	@$(CXX) $(CXXFLAGS) -c $< -o $@


$(buildDir_Stack)/$(target): prepare $(srcObjects)
	cd CustomAssert && make $(makeMode)
	cd CustomAssert/ColorConsole && make $(makeMode)

	$(eval objects = $(srcObjects) $(libObjects))

	@echo [CXX] $(objects) -o $@
	@$(CXX) $(CXXFLAGS) $(objects) -o $@

prepare:
	@mkdir -p $(outDir)
	@mkdir -p $(buildDir_Stack)

build_docs:
	doxygen Doxyfile

endif
