#  Copyright 2007 Vesselin Velichkov, Miroslav Knezevic
# 
#     This file is part of OC.
# 
#     OC is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version.
# 
#     OC is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
# 
#     You should have received a copy of the GNU General Public License
#     along with OC.  If not, see <http://www.gnu.org/licenses/>.
# 
CC = gcc
DEBUG = -g
LFLAGS = -Wall
CFLAGS = -Wall -c
LIB = -lm

oc: oclib.o oc.o
	$(CC) $(LFLAGS) oclib.o oc.o -o oc $(LIB)

oclib.o: 
	$(CC) $(CFLAGS) oclib.c -o oclib.o

oc.o: 
	$(CC) $(CFLAGS) oc.c -o oc.o

clean:
	rm -v *.o
