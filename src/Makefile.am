bin_PROGRAMS=gamesbot gamesbot_mkpasswd

gamesbot_SOURCES=gamesbot.cpp commands.cpp keys.cpp main.cpp configuration.cpp database.cpp highscore.cpp timers.cpp
gamesbot_LDADD=-lrsl_net_irc -lrsl_net_socket -lrsl_file_ini -lpthread -lsqlite3 -ldl

gamesbot_mkpasswd_SOURCES=mkpasswd.cpp keys.cpp

AM_CPPFLAGS=-g -I. -I.. -I../include -pthread -pipe -Wall -DSYSCONFDIR=\"@sysconfdir@\" -DGAMESDIR=\"@gamesdir@\"
AM_LDFLAGS=-Wl,-export-dynamic
