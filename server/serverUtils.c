/*
 * serverUtils.c - support module for server.c
 *
 * Dartmouth CS50, Winter 2021
 */

#include "serverUtils.h"

bool validateAction(char *keyPress, player_t *player, serverInfo_t *info)
{

	position_t *nextPos = malloc(sizeof(position_t));
	nextPos->x = player->pos->x;
	nextPos->y = player->pos->y;

	switch (keyPress[0]){
		case 'h': // Left
			nextPos->x -= 1;
			break;
		case 'l': // Right
			nextPos->x += 1;
			break;
		case 'j': // Down
			nextPos->y += 1;
			break;
		case 'k': // Up
			nextPos->y -= 1;
			break;
		case 'y': // Up Left
			nextPos->x -= 1;
			nextPos->y -= 1;
			break;
		case 'u': // Up Right
			nextPos->x += 1;
			nextPos->y -= 1;
			break;
		case 'b': // Down Left
			nextPos->x -= 1;
			nextPos->y += 1;
			break;
		case 'n': // Down Right
			nextPos->x += 1;
			nextPos->y += 1;
			break;


		case 'H': // Left AFAP
			nextPos->x -= 1000;
			break;
		case 'L': // Right AFAP
			nextPos->x += 1000;
			break;
		case 'J': // Down AFAP
			nextPos->y += 1000;
			break;
		case 'K': // Up AFAP
			nextPos->y -= 1000;
			break;
		case 'Y': // Up Left AFAP
			nextPos->x -= 1000;
			nextPos->y -= 1000;
			break;
		case 'U': // Up Right AFAP
			nextPos->x += 1000;
			nextPos->y -= 1000;
			break;
		case 'B': // Down Left AFAP
			nextPos->x -= 1000;
			nextPos->y += 1000;
			break;
		case 'N': // Down Right AFAP
			nextPos->x += 1000;
			nextPos->y += 1000;
			break;
	}

	// Check the move player 
	map_movePlayer(info->map, player, nextPos);

	return true;
}