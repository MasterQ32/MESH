


mesh-net: mesh-net.htm img/serialnode.png img/hubnode.png img/endnode.png

publish: mesh-net
	scp mesh-net.htm strato:public
	scp style.css strato:public
	scp img/serialnode.png strato:public/img
	scp img/endnode.png    strato:public/img
	scp img/hubnode.png    strato:public/img
	
%.htm: %.md
	render $^ $@

img/%.png: img-src/%.dot
	dot -Tpng -o $@ $^
