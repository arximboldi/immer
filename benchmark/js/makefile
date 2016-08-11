
BOOST=.
EMCXX=em++
EMCXXFLAGS=-O3 -std=c++14 --bind \
	-s TOTAL_MEMORY=536870912 \
	-I ../.. \
	-I $(BOOST)

ALL= \
	out/immu.asmjs.js \
	out/immu.asmjs.html \
	out/immu.wasm.js \
	out/immu.wasm.html

all: $(ALL)

out/immu.asmjs.js: immu.cpp
	@mkdir -p $(@D)
	$(EMCXX) $(EMCXXFLAGS) $< -o $@

# EMCC_WASM_BACKEND=1
out/immu.wasm.js: immu.cpp
	@mkdir -p $(@D)
	$(EMCXX) $(EMCXXFLAGS) -s BINARYEN=1 $< -o $@

%.html: %.js index.tpl.html
	sed s/%IMMU_JS%/`basename $<`/ < index.tpl.html > $@

clean:
	rm -f $(ALL)
