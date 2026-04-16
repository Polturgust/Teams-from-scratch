##
## EPITECH PROJECT, 2025
## G-NWP-400-NCE-4-1-myteams-8
## File description:
## Makefile
##

CXX			=	g++
CXXFLAGS	=	-Wall -Wextra -Werror -std=c++17 -I./include -I./libs/myteams

SRC_SERVER	=	src/server/main.cpp \
				src/server/Server.cpp \
				src/server/handlers/mtp_auth.cpp \
				src/server/handlers/mtp_users.cpp \
				src/server/handlers/mtp_messages.cpp \
				src/server/handlers/mtp_subscriptions.cpp \
				src/server/handlers/mtp_context.cpp \
				src/server/handlers/mtp_create.cpp \
				src/server/handlers/mtp_list.cpp \
				src/server/handlers/mtp_info.cpp

SRC_CLIENT	=	src/client/main.cpp \
				src/client/Client.cpp \
				src/client/Constructor.cpp \
				src/client/Network.cpp \
				src/client/Packet.cpp \
				src/client/stdin_handling.cpp \
				src/client/Command_parser.cpp \
				src/client/Command_dispatch.cpp \
				src/client/Command_help.cpp \
				src/client/Individual.cpp \
				src/client/Server_response.cpp \
				src/client/helpers.cpp

OBJ_SERVER	=	$(SRC_SERVER:.cpp=.o)
OBJ_CLIENT	=	$(SRC_CLIENT:.cpp=.o)

SERVER		=	myteams_server
CLIENT		=	myteams_cli

LDFLAGS		=	-L./libs/myteams -Wl,-rpath,'$$ORIGIN/libs/myteams'
LDLIBS_SRV	=	-lmyteams -luuid
LDLIBS_CLI	=	-lmyteams -luuid

all:		$(SERVER) $(CLIENT)

$(SERVER):	$(OBJ_SERVER)
	$(CXX) $(CXXFLAGS) -o $(SERVER) $(OBJ_SERVER) $(LDFLAGS) $(LDLIBS_SRV)

$(CLIENT):	$(OBJ_CLIENT)
	$(CXX) $(CXXFLAGS) -o $(CLIENT) $(OBJ_CLIENT) $(LDFLAGS) $(LDLIBS_CLI)

clean:
	rm -f $(OBJ_SERVER) $(OBJ_CLIENT)

fclean:		clean
	rm -f $(SERVER) $(CLIENT)

re:			fclean all

.PHONY:		all clean fclean re