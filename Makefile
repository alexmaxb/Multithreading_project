makeprogram: multi-lookup.c util.c
	gcc -pthread -Wall -Wextra -g -o multi-lookup  multi-lookup.c util.c
clean:
	$(RM) multi-lookup
	$(RM) results.txt
	$(RM) serviced.txt
