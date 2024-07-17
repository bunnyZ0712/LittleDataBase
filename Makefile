all=$(wildcard ./App/*.cpp)
obj=$(patsubst ./App/%.cpp, ./Debug/%.o, $(all))

ALL:main.out

LIBS = -lpthread
HEADER = -I./Header
 
main.out:$(obj)
	g++ $^ -o $@ $(LIBS) -g

./Debug/%.o:./App/%.cpp
	g++ -c $^ -o $@ $(HEADER)

./PHONY:
clean:
	rm -rf main.out $(obj)