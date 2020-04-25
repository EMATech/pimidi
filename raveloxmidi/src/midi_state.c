/*
   This file is part of raveloxmidi.

   Copyright (C) 2020 Dave Kelly

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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "config.h"

#include "midi_state.h"
#include "midi_command.h"
#include "data_table.h"
#include "utils.h"

#include "logging.h"

// Sec 3.2 of RFC6295
// As we note above, the first channel command in the MIDI list MUST
// include a status octet.  However, the corresponding command in the
//original MIDI source data stream might not have a status octet (in
// this case, the source would be coding the command using running
// status)
static unsigned char running_status = 0;

void midi_state_lock( midi_state_t *state )
{
	if( ! state ) return;

	pthread_mutex_lock( &(state->lock) );
}

void midi_state_unlock( midi_state_t *state )
{
	if( ! state ) return;

	pthread_mutex_unlock( &(state->lock) );
}

midi_state_t *midi_state_create( size_t buffer_size )
{
	midi_state_t *new_midi_state = NULL;

	new_midi_state = (midi_state_t *)malloc( sizeof(midi_state_t) );

	if( ! new_midi_state )
	{
		logging_printf( LOGGING_ERROR, "midi_state_create: Insufficient memory to allocate new midi state\n");
		return NULL;
	}

	new_midi_state->status = MIDI_STATE_INIT;
	new_midi_state->hold = dbuffer_create( DBUFFER_DEFAULT_BLOCK_SIZE );
	new_midi_state->ring = ring_buffer_create( buffer_size );
	new_midi_state->running_status = 0;
	pthread_mutex_init( &(new_midi_state->lock) , NULL);

	return new_midi_state;
}

void midi_state_reset( midi_state_t *state )
{
	if( ! state ) return;

	midi_state_lock( state );

	state->status = MIDI_STATE_INIT;
	state->running_status = 0;

	if( state->hold )
	{
		dbuffer_reset( state->hold );
	}

	if( state->ring )
	{
		size_t ring_buffer_size = ring_buffer_get_size( state->ring );
		ring_buffer_reset( state->ring , ring_buffer_size );
	}

	midi_state_unlock( state );
}

int midi_state_write( midi_state_t *state, char *in_buffer, size_t in_buffer_len )
{
	int ret = 0;

	if( ! state ) return ret;
	if( ! state->ring ) return ret;
	if( ! in_buffer ) return ret;
	if ( in_buffer_len == 0 ) return ret;

	midi_state_lock( state );

	ring_buffer_write( state->ring, in_buffer, in_buffer_len );

	midi_state_unlock( state );
	
	return 1;
}

void midi_state_destroy( midi_state_t **state )
{

	if( ! state ) return;
	if( ! *state ) return;

	midi_state_lock( (*state) );
	if( (*state)->ring )
	{
		ring_buffer_destroy( &( (*state)->ring ) );
		(*state)->ring = NULL;
	}

	if( (*state)->hold )
	{
		dbuffer_destroy( &( (*state)->hold ) );
		(*state)->hold = NULL;
	}

	midi_state_unlock( (*state) );
	pthread_mutex_destroy( &( (*state)->lock ) );

	free( (*state) );
	*state = NULL;
}

int midi_state_compare( midi_state_t *state, const char *compare, size_t compare_len )
{
	if( ! state ) return -1;
	if( ! state->ring ) return -1;

	return ring_buffer_compare( state->ring, compare, compare_len );
}

int midi_state_char_compare( midi_state_t *state, char compare, size_t index )
{
	if(! state ) return -1;
	if( ! state->ring ) return -1;

	return ring_buffer_char_compare( state->ring, compare, index );
}

char *midi_state_drain( midi_state_t *state, size_t *len)
{
	char *return_buffer = NULL;

	if( ! state ) return NULL;
	if( ! len ) return NULL;

	midi_state_lock( state );

	*len = 0;
	if( ! state->ring ) goto midi_state_drain_end;

	return_buffer = ring_buffer_drain( state->ring, len );

midi_state_drain_end:
	midi_state_unlock( state );

	return return_buffer;
}

void midi_state_advance( midi_state_t *state, size_t steps )
{
	if( ! state ) return;

	midi_state_lock( state );
	if( ! state->ring ) goto midi_state_advance_end;
	
	ring_buffer_advance( state->ring, steps );

midi_state_advance_end:
	midi_state_unlock( state );
}

int midi_state_read_byte( midi_state_t *state, uint8_t *byte )
{
	if( ! byte )
	{
		return -1;
	}

	if( ! state )
	{
		*byte = 0;
		return -1;
	}
}

midi_command_t *midi_state_get_command( midi_state_t *state )
{
	midi_command_t *command = NULL;
	
	if(! state ) goto midi_state_get_command_return;

midi_state_get_command_return:
	return command;
}

const char *midi_status_to_string( midi_state_status_t status )
{
	switch( status )
	{
		case MIDI_STATE_INIT: return "MIDI_STATE_INIT";
		case MIDI_STATE_STATUS_RECEIVED: return "MIDI_STATE_STATUS_RECEIVED";
		case MIDI_STATE_WAIT_BYTE_1: return "MIDI_STATE_WAIT_BYTE_1";
		case MIDI_STATE_WAIT_BYTE_2: return "MIDI_STATE_WAIT_BYTE_2";
		case MIDI_STATE_WAIT_END_SYSEX: return "MIDI_STATE_WAIT_END_SYSEX";
		default: return "Unknown";
	}
	return "Unknown";
}

void midi_state_dump( midi_state_t *state )
{
	if( ! state ) return;

	DEBUG_ONLY;

	logging_printf( LOGGING_DEBUG, "midi_state=%p, status=[%s], running_status=0x%02x\n", 
		state, midi_status_to_string( state->status ), state->running_status);

	ring_buffer_dump( state->ring );
	dbuffer_dump( state->hold );
}

void midi_state_to_commands( midi_state_t *state , data_table_t **command_table)
{
	uint8_t byte = 0;
	uint8_t current_command = 0;
	unsigned char bytes_needed = 0;

	if( ! state ) return;

	*command_table = data_table_create("midi_commands", midi_command_destroy, midi_command_dump );

	while( 1 )
	{
		// If no bytes available...return

		// Read 1 byte

		switch( state->status )
		{
			MIDI_STATE_INIT:
				if( byte < 0x80 )
				{
					// If this is a data byte but there is no running status, we need to loop
					if( state->running_status == 0 ) continue;
					
					// Using the running status, determine how many bytes we need
					bytes_needed = midi_command_bytes_needed( state->running_status );
					
					// Copy the running status into the hold buffer
					// Copy the byte into the hold buffer

					if( bytes_needed == 1)
					{
						state->status = MIDI_STATE_COMMAND_RECEIVED;
						break;
					}
	
					if( bytes_needed == 2 )
					{
						state->status = MIDI_STATE_WAIT_BYTE_1;
						break;
					}
				} else {
					// This is a command byte
					// Copy command byte to hold buffer
	
					// Special cases for SysEx
					if( byte == 0xF0 )
					{
						state->status = MIDI_STATE_WAIT_END_SYSEX;
						break;
					}

					bytes_needed = midi_command_bytes_needed( byte & 0xF0 );

					switch( bytes_needed )
					{
						case 1:
							state->status = MIDI_STATE_WAIT_BYTE_1;
							break;
						case 2:
							state->status = MIDI_STATE_WAIT_BYTE_2;
							break;
						case 0:
						default:
							state->status = MIDI_STATE_COMMAND_RECEIVED;
							break;
					}
				}
				break;
			MIDI_STATE_WAIT_BYTE_2:
				// Copy byte to hold buffer
				state->status = MIDI_STATE_WAIT_BYTE_1;
				break;
			MIDI_STATE_WAIT_BYTE_1:
				// Copy byte to hold buffer
				state->status = MIDI_STATE_COMMAND_RECEIVED;
				break;
			MIDI_STATE_WAIT_END_SYSEX:
				// Copy byte to hold buffer
				if( byte == 0xF7 )
				{
					state->status = MIDI_STATE_COMMAND_RECEIVED;
				}
				break;
		}


		// If we're waiting for more data, loop around
		if( state->status != MIDI_STATE_COMMAND_RECEIVED) continue;

		// Take the full command from the hold buffer
		// Add it to the command list
	}
}
