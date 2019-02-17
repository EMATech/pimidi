/*
   This file is part of raveloxmidi.

   Copyright (C) 2014 Dave Kelly

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation,
   Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA 
*/

#ifndef NET_APPLEMIDI_H
#define NET_APPLEMIDI_H

#define NET_APPLEMIDI_DONE	0
#define NET_APPLEMIDI_NEED_DATA	1
#define NET_APPLEMIDI_NO_MEMORY	2

#define NET_APPLEMIDI_CMD_INV		0x494e
#define NET_APPLEMIDI_CMD_SYNC		0x434b
#define NET_APPLEMIDI_CMD_ACCEPT	0x4f4b
#define NET_APPLEMIDI_CMD_REJECT	0x4e4f
#define NET_APPLEMIDI_CMD_END		0x4259
#define NET_APPLEMIDI_CMD_FEEDBACK	0x5253
#define NET_APPLEMIDI_CMD_BITRATE	0x524c
#define NET_APPLEMIDI_CMD_UNKOWN	0x1111

#define NET_APPLEMIDI_COMMAND_SIZE	4
#define NET_APPLEMIDI_INV_STATIC_SIZE	16	
#define NET_APPLEMIDI_SYNC_SIZE		36
#define NET_APPLEMIDI_FEEDBACK_SIZE	8
#define NET_APPLEMIDI_BITRATE_SIZE	8

#define NET_APPLEMIDI_UDPSIZE		1472

typedef struct net_applemidi_command {
	uint16_t	signature;
	uint16_t	command;
	void 		*data;
} net_applemidi_command;

typedef struct net_applemidi_inv {
	uint32_t	ssrc;
	uint32_t	version;
	uint32_t	initiator;
	char	*name;
} net_applemidi_inv;

typedef struct net_applemidi_sync {
	uint32_t	ssrc;
	char	count;
	char	padding[3];
	uint64_t	timestamp1;
	uint64_t	timestamp2;
	uint64_t	timestamp3;
} net_applemidi_sync;

typedef struct net_applemidi_feedback {
	uint32_t		ssrc;
	union {
		uint32_t	apple_seq;
		uint16_t	rtp_seq[2];
	};
} net_applemidi_feedback;

typedef struct net_applemidi_bitrate {
	uint32_t	ssrc;
	uint32_t	limit;
} net_applemidi_bitrate;

void net_applemidi_command_dump( net_applemidi_command *command);
net_applemidi_inv * net_applemidi_inv_create( void );
net_applemidi_sync * net_applemidi_sync_create( void );
net_applemidi_feedback * net_applemidi_feedback_create( void );
int net_applemidi_cmd_destroy( net_applemidi_command **command );
int net_applemidi_unpack( net_applemidi_command **command_buffer, unsigned char *in_buffer, size_t in_buffer_len);
int net_applemidi_pack( net_applemidi_command *command_buffer, unsigned char **out_buffer, size_t *out_buffer_len );
net_applemidi_command * net_applemidi_cmd_create( uint16_t command );

void net_applemidi_inv_destroy( net_applemidi_inv **inv );

#endif
