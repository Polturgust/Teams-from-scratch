##
## EPITECH PROJECT, 2025
## G-NWP-400-NCE-4-1-myteams-8
## File description:
## Makefile
##

CXX			=	g++
CXXFLAGS	=	-Wall -Wextra -Werror -std=c++17 -I./include -I./libs/myteams

SRC_SERVER	=	src/server/main.cpp

SRC_CLIENT	=	src/client/main.cpp

OBJ_SERVER	=	$(SRC_SERVER:.cpp=.o)
OBJ_CLIENT	=	$(SRC_CLIENT:.cpp=.o)

SERVER		=	myteams_server
CLIENT		=	myteams_cli

LDFLAGS		=	-L./libs/myteams
LDLIBS_SRV	=	-lmyteams -luuid
LDLIBS_CLI	=	-lmyteams -luuid

all:	$(SERVER) $(CLIENT)

$(SERVER):	$(OBJ_SERVER)
	$(CXX) $(CXXFLAGS) -o $(SERVER) $(OBJ_SERVER) $(LDFLAGS) $(LDLIBS_SRV)

$(CLIENT):	$(OBJ_CLIENT)
	$(CXX) $(CXXFLAGS) -o $(CLIENT) $(OBJ_CLIENT) $(LDFLAGS) $(LDLIBS_CLI)

clean:
	rm -f $(OBJ_SERVER) $(OBJ_CLIENT)

fclean:	clean
	rm -f $(SERVER) $(CLIENT)

re:	fclean all

.PHONY:	all clean fclean re