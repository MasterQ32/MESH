
all: client.exe

%.exe: $(wildcard *.cs)
	mcs -out:$@ -main:$(shell echo $* | python2 -c "print raw_input().capitalize()") $^

