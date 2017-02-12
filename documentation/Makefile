


mesh-network: mesh-network.htm img/serialnode.png img/hubnode.png img/endnode.png

publish: mesh-network
	scp mesh-network.htm strato:public
	scp style.css strato:public
	scp img/serialnode.png strato:public/img
	scp img/endnode.png    strato:public/img
	scp img/hubnode.png    strato:public/img
	
%.htm: %.md
	# Render the markdown file with github markdown
	render $^ $@

img/%.png: img-src/%.dot
	dot -Tpng -o $@ $^
