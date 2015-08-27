CC      = gcc
C_FLAGS = -I/usr/include/mysql
C_FLAGS += -Wall -Wextra -g3 -ggdb3 $(PROF) $(NOCRYPT)
L_FLAGS = -L/usr/local/lib -lpthread -L/usr/lib64/mysql -lmysqlclient
DATE = `date +%m%d%y`

O_FILES = \
	bserver.o \
	buffer_list.o \
	command.o \
	connection.o \
	epoll.o \
	main.o \
	mysql.o \
	profile.o \
	smart_buf.o \
	trie.o \
	util.o

bserver: $(O_FILES)
	rm -f bserver
	$(CC) -o bserver $(O_FILES) $(L_FLAGS)
  
# make love, not distclean
love: $(O_FILES)
	@echo "<3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3"
	rm -f bserver
	$(CC) -o bserver $(O_FILES) $(L_FLAGS)
	@echo "<3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3 <3"
  
.c.o:
	$(CC) -c $(C_FLAGS) $<
  
clean:
	rm *.o
	touch *.[ch]
	make
  
tar:
	rm -rf *.o
	tar czhf ../bserver$(DATE).tgz *.*

