
BOOST=.
EMCXX=em++
EMCXXFLAGS=-O3 -std=c++14 --bind \
	-s TOTAL_MEMORY=536870912 \
	-I ../.. \
	-I $(BOOST)

ALL= \
	out/immer.asmjs.js \
	out/immer.asmjs.html \
	out/immer.wasm.js \
	out/immer.wasm.html

all: $(ALL)

out/immer.asmjs.js: immer.cpp
	@mkdir -p $(@D)
	$(EMCXX) $(EMCXXFLAGS) $< -o $@

out/immer.wasm.js: immer.cpp
	@mkdir -p $(@D)
	EMCC_WASM_BACKEND=1 $(EMCXX) $(EMCXXFLAGS) -s BINARYEN=1 $< -o $@

%.html: %.js index.tpl.html
	sed s/%IMMER_JS%/`basename $<`/ < index.tpl.html > $@

clean:
	rm -f $(ALL)
