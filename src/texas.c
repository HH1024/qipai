﻿#include "texas.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "sort_card.h"
#include "ht_lch.h"

static int texas_table_rank[16] = { 0, 14, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 0, 0 };
static int texas_table_logic[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 1, 0 };
static int table_suit[6] = { 0, 1, 2, 3, 4, 5 };
static const int straight[10] = { 7936, 3968, 1984, 992, 496, 248, 124, 62, 31, 4111 };

void _deck_init(texas_t* texas)
{
    int i,j,n;

    n = 0;
    for(i = cdSuitDiamond; i <= cdSuitSpade; ++i){
        for(j = cdRankAce; j <= cdRankK; ++j){
            texas->deck[n].rank = j;
            texas->deck[n].suit = i;
            n++;
        }
    }
    texas->deal_index = 0;
}

void _deck_shuffle(texas_t* texas)
{
    int i,n;
    int a,b;
    card_t temp;
    card_t *pa, *pb;

    n = 1000 + rand() % 50;
    for(i = 0; i < n; ++i){
        a = rand() % TEXAS_DECK_NUM;
        b = rand() % TEXAS_DECK_NUM;
        if(a != b){
            pa = &texas->deck[a];
            pb = &texas->deck[b];
            temp.rank = pa->rank;
            temp.suit = pa->suit;
            pa->rank = pb->rank;
            pa->suit = pb->suit;
            pb->rank = temp.rank;
            pb->suit = temp.suit;
        }
    }
    texas->deal_index = 0;
}

int _deck_deal(texas_t* texas, card_t* card)
{
    if(!texas || !card)
        return HTERR_PARAM;

    if(texas->deal_index >= TEXAS_DECK_NUM)
        return HTERR_NOCARD;

    card->rank = texas->deck[texas->deal_index].rank;
    card->suit = texas->deck[texas->deal_index].suit;
    texas->deal_index++;

    return 0;
}

void texas_init(texas_t* texas)
{
    int i,j;

    if(!texas)
        return;
    _deck_init(texas);
    texas->round = 0;
    texas->b_jump_end = 0;
    texas->b_burn = 1;
    texas_set_blind(texas, 1);
    texas->debug = 0;
    texas->game_state = TEXAS_GAME_END;
    texas->last_state = texas->game_state;
    texas->inning = 0;
    texas->turn_time = 30;
    texas->curr_turn_time = 30;
    texas->dealer_player_no = 0;
    texas->small_blind_no = 0;
    texas->big_blind_no = 0;
    texas->first_player_no = 0;
    texas->curr_player_no = 0;
    texas->largest_player_no = 0;
    texas->curr_poti = 0;
    texas->player_num = 0;

    for(i = 0; i < 5; i++){
        texas->board[i].rank = 0;
        texas->board[i].suit = 0;
    }

    for(i = 0; i < TEXAS_MAX_PLAYER; i++){
        texas->players[i].state = 0;
        texas->players[i].gold = 0;
        texas->players[i].score = 0;
        texas->players[i].param1 = 0;
        texas->players[i].param2 = 0;
        texas->players[i].card_num = 0;
        for(j = 0; j < TEXAS_MAX_CARDS; ++j){
            texas->players[i].mycards[j].rank = 0;
            texas->players[i].mycards[j].suit = 0;
        }
        texas->players[i].mytype.name = 0;
        texas->players[i].mytype.param1 = 0;
        for(j = 0; j < 5; j++){
            texas->players[i].mybest[j].rank = 0;
            texas->players[i].mybest[j].suit = 0;
        }

        texas->pots[i].total_chip = 0;
        for(j = 0; j < TEXAS_MAX_PLAYER; j++){
            texas->pots[i].player_chip[j] = 0;
            texas->pots[i].win_flag[j] = 0;
        }
    }
}

void texas_start(texas_t* texas)
{
    int i,j,k;
    card_t card;

    if(!texas)
        return;
    if(texas->player_num < 2)
        return;
    if(texas->small_blind <= 0)
        return;
    for(i = 0;i < texas->player_num; i++){
        if(texas->players[i].gold < (texas->small_blind * 2))
            return;
    }

    _deck_shuffle(texas);
    texas->round = 0;
    texas->curr_poti = 0;
    texas->b_jump_end = 0;
    texas->game_state = TEXAS_GAME_PREFLOP;
    texas->last_state = texas->game_state;
    for(i = 0; i < TEXAS_MAX_PLAYER; ++i){
        texas->players[i].state = PLAYER_ACTION_WAIT;
        texas->players[i].card_num = 0;
        for(j = 0; j < TEXAS_MAX_CARDS; ++j){
            texas->players[i].mycards[j].rank = 0;
            texas->players[i].mycards[j].suit = 0;
        }
        texas->players[i].mytype.name = TEXAS_HIGHCARD;
        texas->players[i].mytype.param1 = 0;
        for(j = 0; j < 5; j++){
            texas->players[i].mybest[j].rank = 0;
            texas->players[i].mybest[j].suit = 0;
        }

        texas->pots[i].total_chip = 0;
        for(j = 0; j < TEXAS_MAX_PLAYER; ++j){
            texas->pots[i].player_chip[j] = 0;
            texas->pots[i].win_flag[j] = 0;
        }
    }

    /* the button position */
    if(!texas->inning)
        texas->dealer_player_no = rand() % texas->player_num;
    else{
        texas->dealer_player_no++;
        if(texas->dealer_player_no >= texas->player_num)
            texas->dealer_player_no = 0;
    }
    /* small,big position */
    if(texas->player_num == 2){
        /* heads-up(1v1) */
        texas->small_blind_no = texas->dealer_player_no;
    }
    else{
        texas->small_blind_no = texas->dealer_player_no + 1;
        if(texas->small_blind_no >= texas->player_num)
            texas->small_blind_no = 0;
    }
    texas->big_blind_no = texas->small_blind_no + 1;
    if(texas->big_blind_no >= texas->player_num)
        texas->big_blind_no = 0;
    /* first position */
    if(texas->player_num == 2)
        texas->first_player_no = texas->small_blind_no;
    else{
        texas->first_player_no = texas->big_blind_no + 1;
        if(texas->first_player_no >= texas->player_num)
            texas->first_player_no = 0;
    }
    texas->curr_player_no = texas->first_player_no;

    /* about chip */
    texas->turn_max_chip = texas->small_blind * 2;
    texas->pots[0].total_chip = 3 * texas->small_blind;
    texas->pots[0].player_chip[texas->small_blind_no] = texas->small_blind;
    texas->pots[0].player_chip[texas->big_blind_no] = texas->small_blind * 2;
    texas->players[texas->small_blind_no].gold -= texas->small_blind;
    texas->players[texas->big_blind_no].gold -= texas->turn_max_chip;

    /* draw two cards for every player */
    for(i = 0; i < 2; ++i){
        k = texas->dealer_player_no + 1;
        if(k >= texas->player_num)
            k = 0;
        for(j = 0; j < texas->player_num; ++j){
            _deck_deal(texas, &texas->players[k].mycards[i]);
            texas->players[k].card_num++;
            k++;
            if(k >= texas->player_num)
                k = 0;
        }
    }

    /* draw five board cards */
    if(texas->b_burn){
        _deck_deal(texas, &card);
        for(i = 0; i < 3; ++i){
            _deck_deal(texas, &texas->board[i]);
        }
        _deck_deal(texas, &card);
        _deck_deal(texas, &texas->board[3]);
        _deck_deal(texas, &card);
        _deck_deal(texas, &texas->board[4]);
    }
    else{
        for(i = 0; i < 5; ++i){
            _deck_deal(texas, &texas->board[i]);
        }
    }

    /* add public 5 cards to player */
    for(i = 0; i < texas->player_num; ++i){
        for(j = 0; j < 5; j++){
            texas->players[i].mycards[2+j].rank = texas->board[j].rank;
            texas->players[i].mycards[2+j].suit = texas->board[j].suit;
        }
        texas->players[i].card_num += 5;
    }

    texas->inning++;
}

void texas_set_burn(texas_t* texas, int burn)
{
    if(!texas)
        return;

    texas->b_burn = burn;
}

void texas_set_blind(texas_t* texas, unsigned int chip)
{
    if(!texas)
        return;

    texas->small_blind = chip;
    texas->min_raise = 2 * texas->small_blind;
}

uint64_t texas_player_bet(texas_t* texas, int player_no)
{
    int i;
    uint64_t chip;

    if(player_no >= texas->player_num)
        return 0;

    chip = 0;
    for(i = 0; i <= texas->curr_poti; i++){
        chip += texas->pots[i].player_chip[player_no];
    }

    return chip;
}

uint64_t texas_player_win(texas_t* texas, int player_no)
{
    int i;
    uint64_t chip;

    if(!texas)
        return 0;
    if(player_no >= texas->player_num)
        return 0;

    chip = 0;
    for(i = 0; i <= texas->curr_poti; i++){
        chip += texas_pot_win(texas, i, player_no);
    }

    return chip;
}

uint64_t texas_pot_win(texas_t* texas, int pot_index, int player_no)
{
    int i,n;
    uint64_t chip;

    if(!texas)
        return 0;
    if(pot_index > texas->curr_poti)
        return 0;
    if(player_no >= texas->player_num)
        return 0;

    if(texas->pots[pot_index].win_flag[player_no]){
        n = 0;
        for(i = 0; i < TEXAS_MAX_PLAYER; i++){
            if(texas->pots[pot_index].win_flag[i])
                n++;
        }
        chip = texas->pots[pot_index].total_chip / n;
    }
    else{
        chip = 0;
    }

    return chip;
}

void texas_calc_type(texas_t* texas, int player_no)
{
    int i,j,m,v;
    int sub[5];
    int x[20];
    int su_num[4];
    card_t *p;
    card_t tempc;
    card_t su_cards[4][7];

    if(!texas)
        return;
    if(player_no >= texas->player_num)
        return;

    if(texas->players[player_no].card_num < 5)
        return;

    texas->players[player_no].mytype.name = TEXAS_HIGHCARD;
    texas->players[player_no].mytype.param1 = 0;

    memset(su_cards, 0, sizeof(card_t) * 4 * 7);
    memset(su_num, 0, sizeof(int) * 4);
    memset(sub, 0, sizeof(int) * 5);

    for(i = 0; i < texas->players[player_no].card_num; ++i){
        p = &texas->players[player_no].mycards[i];
        su_cards[p->suit-1][su_num[p->suit-1]].rank = p->rank;
        su_cards[p->suit-1][su_num[p->suit-1]].suit = p->suit;
        su_num[p->suit-1]++;
        v = texas_logicvalue(p);
        x[v]++;
    }

    /* Royal Straight Flush */
    /* Straight Flush */
    for(i = 0; i < 4; i++){
        if(su_num[i] < 5) continue;
        memset(x, 0, sizeof(int) * 20);
        for(j = 0; j < su_num[i]; ++j){
            v = texas_logicvalue(&su_cards[i][j]);
            x[v]++;
        }
        for(j = 14; j >= 6; j--){
            if(x[j] && x[j-1] && x[j-2] && x[j-3] && x[j-4]){
                /* best hand */
                for(m = 0; m < 5; m++){
                    texas->players[player_no].mybest[m].rank = x[j+m];
                    texas->players[player_no].mybest[m].suit = i+1;
                }

                if(j == 14){
                    /* Royal Flush */
                    texas->players[player_no].mytype.name = TEXAS_ROYAL;
                    return;
                }
                else{
                    texas->players[player_no].mytype.name = TEXAS_STRAIGHT_FLUSH;
                    texas->players[player_no].mytype.param1 = j;
                    return;
                }
            }
        }
        /* Flush */
        texas->players[player_no].mytype.name = TEXAS_FLUSH;
        m = 0;
        for(j = 14; j >= 0; j--){
            if(x[j]){
                sub[m] = j;
                m++;
                if(m >= 5) break;
            }
        }
        texas->players[player_no].mytype.param1 = sub[0] << 16 | sub[1] << 12 |
            sub[2] << 8 | sub[3] << 4 | sub[4];

        /* best hand */
        m = 0;
        for(j = 14; j >= 0; j--){
            if(x[j] == 0) continue;
            texas->players[player_no].mybest[m].rank = texas_rankvalue(j);
            texas->players[player_no].mybest[m].suit = i+1;
            m++;
            if(m >= 5) break;
        }
        return;
    }

    /* four of kind */
    memset(x, 0, sizeof(int) * 20);
    for(i = 0; i < texas->players[player_no].card_num; ++i){
        v = texas_logicvalue(&texas->players[player_no].mycards[i]);
        x[v]++;
    }
    for(i = 0; i < 15; i++){
        if(x[i] == 4){
            sub[0] = i;
            texas->players[player_no].mytype.name = TEXAS_FOUR;
            sub[1] = 0;
            for(j = 0; j < texas->players[player_no].card_num; ++j){
                v = texas_logicvalue(&texas->players[player_no].mycards[j]);
                if(v == i) continue;
                if(v > sub[1]){
                    sub[1] = v;
                    tempc.rank = texas->players[player_no].mycards[j].rank;
                    tempc.suit = texas->players[player_no].mycards[j].suit;
                }
            }
            texas->players[player_no].mytype.param1 = sub[0] << 4 | sub[1];
            /* best hand */
            for(j = 0; j < 4; j++){
                texas->players[player_no].mybest[j].rank = texas_rankvalue(i);
                texas->players[player_no].mybest[j].suit = j + 1;
            }
            texas->players[player_no].mybest[4].rank = tempc.rank;
            texas->players[player_no].mybest[4].suit = tempc.suit;
            return;
        }
    }

    /* straight */
    for(j = 14; j >= 6; j--){
        if(x[j] && x[j-1] && x[j-2] && x[j-3] && x[j-4]){
            texas->players[player_no].mytype.name = TEXAS_STRAIGHT;
            texas->players[player_no].mytype.param1 = j;

            for(m = 0; m < 5; m++){
                for(i = 0; i < texas->players[player_no].card_num; i++){
                    if((j-m) == texas_logicvalue(&texas->players[player_no].mycards[i])){
                        texas->players[player_no].mybest[m].rank = texas->players[player_no].mycards[i].rank;
                        texas->players[player_no].mybest[m].suit = texas->players[player_no].mycards[i].suit;
                        break;
                    }
                }
            }
            return;
        }
    }

    /* full house,three */
    sub[0] = 0;
    for(i = 14; i >= 0; i--){
        if(x[i] == 3){
            sub[0] = i;
            break;
        }
    }
    sub[1] = 0;
    for(i = 14; i >= 0; i--){
        if(x[i] == 2 && i > sub[1]){
            sub[1] = i;
        }
    }
    if(sub[0] && sub[1]){
        texas->players[player_no].mytype.name = TEXAS_FULLHOUSE;
        texas->players[player_no].mytype.param1 = sub[0] << 4 | sub[1];

        m = 0;
        for(j = 0; j < texas->players[player_no].card_num; j++){
            v = texas_logicvalue(&texas->players[player_no].mycards[j]);
            if(sub[0] == v || sub[1] == v){
                texas->players[player_no].mybest[m].rank = texas->players[player_no].mycards[j].rank;
                texas->players[player_no].mybest[m].suit = texas->players[player_no].mycards[j].suit;
                m++;
            }
            if(m >= 5)
                break;
        }
        return;
    }
    if(sub[0] > 0 && sub[1] == 0){
        texas->players[player_no].mytype.name = TEXAS_THREE;
        m = 0;
        sub[1] = sub[2] = 0;
        for(i = 14; i >= 0; i--){
            if(x[i] == 1){
                if(m == 0)
                    sub[1] = i;
                else
                    sub[2] = i;
                m++;
                if(m == 2)
                    break;
            }
        }
        texas->players[player_no].mytype.param1 = sub[0] << 8 | sub[1] << 4 | sub[2];

        m = 0;
        for(j = 0; j < texas->players[player_no].card_num; j++){
            v = texas_logicvalue(&texas->players[player_no].mycards[j]);
            if(v == sub[0] || v == sub[1] || v == sub[2]){
                texas->players[player_no].mybest[m].rank = texas->players[player_no].mycards[j].rank;
                texas->players[player_no].mybest[m].suit = texas->players[player_no].mycards[j].suit;
                m++;
            }
            if(m >= 5)
                break;
        }
        return;
    }

    /* pair */
    j = 0;
    memset(sub, 0, sizeof(int) * 5);
    for(i = 14; i >= 0; i--){
        if(x[i] == 2){
            j++;
            if(sub[0] == 0) sub[0] = i;
            else sub[1] = i;
            if(j == 2)
                break;
        }
    }
    if(j == 2){
        texas->players[player_no].mytype.name = TEXAS_PAIR2;
        sub[2] = 0;
        for(i = 14; i >= 0; i--){
            if(x[i] == 1){
                sub[2] = i;
                break;
            }
        }
        texas->players[player_no].mytype.param1 = sub[0] << 8 | sub[1] << 4 | sub[2];

        m = 0;
        for(i = 0; i < texas->players[player_no].card_num; i++){
            v = texas_logicvalue(&texas->players[player_no].mycards[i]);
            if(sub[0] == v || sub[1] == v || sub[2] == v){
                texas->players[player_no].mybest[m].rank = texas->players[player_no].mycards[i].rank;
                texas->players[player_no].mybest[m].suit = texas->players[player_no].mycards[i].suit;
                m++;
            }
            if(m >= 5)
                break;
        }
        return;
    }
    if(j == 1){
        texas->players[player_no].mytype.name = TEXAS_PAIR1;
        m = 0;
        for(i = 19; i >= 0; i--){
            if(x[i] == 1){
                if(m == 0)
                    sub[1] = i;
                else if(m == 1)
                    sub[2] = i;
                else
                    sub[3] = i; 
                m++;
                if(m == 3)
                    break;
            }
        }
        texas->players[player_no].mytype.param1 = sub[0] << 12 | sub[1] << 8 | sub[2] << 4 | sub[3];

        m = 0;
        for(j = 0; j < texas->players[player_no].card_num; j++){
            v = texas_logicvalue(&texas->players[player_no].mycards[j]);
            if(v == sub[0] || v == sub[1] || v == sub[2] || v == sub[3]){
                texas->players[player_no].mybest[m].rank = texas->players[player_no].mycards[j].rank;
                texas->players[player_no].mybest[m].suit = texas->players[player_no].mycards[j].suit;
                m++;
            }
            if(m >= 5)
                break;
        }
        return;
    }

    /* high card */
    texas->players[player_no].mytype.name = TEXAS_HIGHCARD;
    m = 0;
    memset(sub, 0, sizeof(int) * 5);
    for(i = 14; i >= 0; i--){
        if(x[i] == 1){
            if(m == 0)
                sub[0] = i;
            else if(m == 1)
                sub[1] = i;
            else if(m == 2)
                sub[2] = i;
            else if(m == 3)
                sub[3] = i;
            else if(m == 4)
                sub[4] = i;
            m++;
            if(m == 5)
                break;
        }
    }
    texas->players[player_no].mytype.param1 = sub[0] << 16 | sub[1] << 12 | sub[2] << 8 |
        sub[3] << 4 | sub[4];

    /* best hand */
    m = 0;
    for(i = 0; i < texas->players[player_no].card_num; i++){
        v = texas_logicvalue(&texas->players[player_no].mycards[i]);
        if(v == sub[0] || v == sub[1] || v == sub[2]
        || v == sub[3] || v == sub[4]){
            texas->players[player_no].mybest[m].rank = texas->players[player_no].mycards[i].rank;
            texas->players[player_no].mybest[m].suit = texas->players[player_no].mycards[i].suit;
            m++;
        }
        if(m >= 5) break;
    }
}

void texas_end(texas_t* texas)
{
    int i,j,n,live_num,flag;
    texas_type ttmax;

    if(!texas)
        return;

    live_num = 0;
    for(i = 0; i < texas->player_num; i++){
        if(texas->players[i].state != PLAYER_ACTION_FOLD)
            live_num++;
    }

    if(texas->game_state >= TEXAS_GAME_POST_RIVER && live_num > 1){
        /* compute live player's type and best hand */
        for(i = 0; i < texas->player_num; i++){
            if(texas->players[i].state != PLAYER_ACTION_FOLD){
                texas_calc_type(texas, i);
            }
        }

        for(i = 0; i <= texas->curr_poti; i++){
            ttmax.name = TEXAS_HIGHCARD;
            ttmax.param1 = 0;
            /* get this pot max hand */
            for(j = 0; j < texas->player_num; j++){
                if(texas->pots[i].player_chip[j] > 0 &&
                    texas->players[j].state != PLAYER_ACTION_FOLD){
                        flag = 0;
                        if(texas->players[j].mytype.name > ttmax.name)
                            flag = 1;
                        else if(texas->players[j].mytype.name == ttmax.name){
                            if(texas->players[j].mytype.param1 > ttmax.param1)
                                flag = 1;
                        }
                        if(flag){
                            memcpy(&ttmax, &texas->players[j].mytype, sizeof(texas_type));
                        }
                }
            }
            /* flag winner */
            n = 0;
            for(j = 0; j < texas->player_num; j++){
                if(texas->pots[i].player_chip[j] > 0 &&
                    texas->players[j].state != PLAYER_ACTION_FOLD){
                        if(texas->players[j].mytype.name == ttmax.name &&
                            texas->players[j].mytype.param1 == ttmax.param1){
                                texas->pots[i].win_flag[j] = 1;
                                n++;
                        }
                }
            }
            /* alloc this pot */
            for(j = 0; j < texas->player_num; j++){
                if(texas->pots[i].win_flag[j] > 0){
                    texas->players[j].gold += texas->pots[i].total_chip / n;
                }
            }
        } // for pot
    }
    else{
        for(i = 0; i < texas->player_num; i++){
            if(texas->players[i].state != PLAYER_ACTION_FOLD){
                texas->pots[0].win_flag[i] = 1;
                texas->players[i].gold += texas->pots[0].total_chip;
            }
        }
    }
}

void texas_next_step(texas_t* texas)
{
    int i,state,loop,allin_num;
    int chip_same,left_num;
    int flag_end;
    uint64_t chip;

    if(!texas)
        return;

    left_num = 0;
    allin_num = 0;
    chip = 0;
    for(i = 0; i < texas->player_num; i++){
        if(texas->players[i].state == PLAYER_ACTION_FOLD)
            continue;
        left_num++;
        if(texas->pots[texas->curr_poti].player_chip[i] > chip)
            chip = texas->pots[texas->curr_poti].player_chip[i];
        if(texas->players[i].state == PLAYER_ACTION_ALLIN)
            allin_num++;
    }

    chip_same = 1;
    for(i = 0; i < texas->player_num; i++){
        if(texas->players[i].state == PLAYER_ACTION_FOLD)
            continue;
        if(texas->players[i].state == PLAYER_ACTION_ALLIN)
            continue;
        if(texas->players[i].state == PLAYER_ACTION_WAIT){
            chip_same = 0;
            break;
        }
        if(texas->pots[texas->curr_poti].player_chip[i] != chip){
            chip_same = 0;
            break;
        }
    }
    if(left_num == 1)
        chip_same = 1;
    if(chip_same){
        loop = TEXAS_MAX_PLAYER + 1;
        while(loop && texas_pot_split(texas)) loop--;

        flag_end = 0;
        if(allin_num == left_num || left_num == 1)
            flag_end = 1;
        if(texas->game_state == TEXAS_GAME_RIVER)
            flag_end = 1;
        if(left_num == (allin_num + 1))
            flag_end = 1;

        if(flag_end)
        {
            /* game end */
            texas->last_state = texas->game_state;
            texas->game_state = TEXAS_GAME_POST_RIVER;
            if(texas->last_state != TEXAS_GAME_RIVER)
                texas->b_jump_end = 1;
            texas_end(texas);
        }
        else{
            /* goto next game state */
            texas->turn_max_chip = 0;
            texas->min_raise = 2 * texas->small_blind;
            texas->last_state = texas->game_state;
            texas->game_state++;
            if(texas->player_num == 2)
                texas->curr_player_no = texas->big_blind_no;
            else
                texas->curr_player_no = texas->small_blind_no;
            for(i = 0; i < texas->player_num; i++){
                if(texas->players[i].state != PLAYER_ACTION_FOLD &&
                    texas->players[i].state != PLAYER_ACTION_ALLIN){
                        texas->players[i].state = PLAYER_ACTION_WAIT;
                }
                if(texas->players[texas->curr_player_no].state == PLAYER_ACTION_FOLD ||
                    texas->players[texas->curr_player_no].state == PLAYER_ACTION_ALLIN){
                        texas->curr_player_no++;
                        if(texas->curr_player_no >= texas->player_num)
                            texas->curr_player_no = 0;
                }
            }
        }
    }
    else{
        for(i = 0; i < texas->player_num; i++){
            texas->curr_player_no++;
            if(texas->curr_player_no >= texas->player_num)
                texas->curr_player_no = 0;
            state = texas->players[texas->curr_player_no].state;
            if(state == PLAYER_ACTION_FOLD || state == PLAYER_ACTION_ALLIN)
                continue;
            if(state == PLAYER_ACTION_CHECK &&
                texas->players[texas->curr_player_no].gold == 0)
                continue;
            break;
        }
    }    
}

int texas_live_num(texas_t* texas)
{
    int i;
    int live = 0;

    for(i = 0; i < texas->player_num; ++i){
        if(texas->players[i].state != PLAYER_ACTION_FOLD)
            live++;
    }

    return live;
}

int texas_pot_split(texas_t* texas)
{
    int split,i,num;
    uint64_t min_chip,max_chip;
    uint64_t temp[TEXAS_MAX_PLAYER];

    /* get max chip from the pot */
    max_chip = min_chip = 0;
    for(i = 0; i < texas->player_num; i++){
        if(texas->pots[texas->curr_poti].player_chip[i] > max_chip)
            max_chip = texas->pots[texas->curr_poti].player_chip[i];
    }

    /* need split? */
    min_chip = max_chip;
    num = 0;
    split = 0;
    for(i = 0; i < texas->player_num; i++){
        if(texas->pots[texas->curr_poti].player_chip[i] > 0)
            num++;
        if(texas->pots[texas->curr_poti].player_chip[i] < min_chip)
            min_chip = texas->pots[texas->curr_poti].player_chip[i];
        if(texas->pots[texas->curr_poti].player_chip[i] < max_chip &&
            texas->players[i].state != PLAYER_ACTION_FOLD &&
            texas->players[i].gold == 0){
                split = 1;
        }
    }
    if(num == 1) split = 0;

    if(split){
        for(i = 0; i < TEXAS_MAX_PLAYER; i++){
            if(texas->pots[texas->curr_poti].player_chip[i] > 0){
                temp[i] = texas->pots[texas->curr_poti].player_chip[i];
                temp[i] -= min_chip;
                texas->pots[texas->curr_poti].player_chip[i] = min_chip;
            }
            else
                temp[i] = 0;
        }
        texas->pots[texas->curr_poti].total_chip = num * min_chip;
        texas->curr_poti++;
        if(texas->curr_poti >= TEXAS_MAX_PLAYER){
            printf("pot out of range when split!\n");
            return 0;
        }

        texas->pots[texas->curr_poti].total_chip = 0;
        for(i = 0; i < TEXAS_MAX_PLAYER; i++){
            texas->pots[texas->curr_poti].player_chip[i] = temp[i];
            texas->pots[texas->curr_poti].total_chip += temp[i];
        }
    }

    return split;
}

uint64_t texas_call_need_chip(texas_t* texas, int player_no)
{
    int i;
    uint64_t chip;

    if(!texas)
        return 0;
    if(player_no >= texas->player_num)
        return 0;

    chip = 0;
    for(i = 0; i < TEXAS_MAX_PLAYER; i++){
        if(texas->pots[texas->curr_poti].player_chip[i] > chip)
            chip = texas->pots[texas->curr_poti].player_chip[i];
    }
    if(chip > texas->pots[texas->curr_poti].player_chip[player_no])
        chip -= texas->pots[texas->curr_poti].player_chip[player_no];
    else
        chip = 0;

    return chip;
}

int texas_fold(texas_t* texas, int player_no)
{
    if(!texas)
        return 0;
    if(player_no >= texas->player_num)
        return 0;
    if(player_no != texas->curr_player_no)
        return 0;

    texas->players[player_no].state = PLAYER_ACTION_FOLD;
    texas_next_step(texas);
    
    return 1;
}

uint64_t texas_bet(texas_t* texas, int player_no, unsigned int chip)
{
    if(!texas)
        return 0;
    if(player_no >= texas->player_num)
        return 0;
    if(player_no != texas->curr_player_no)
        return 0;
    if(texas->turn_max_chip > 0)
        return 0;
    if(chip > texas->players[player_no].gold)
        return 0;
    if(chip < (texas->small_blind * 2))
        return 0;

    texas->turn_max_chip = chip;
    if(chip > texas->min_raise)
        texas->min_raise = chip;
    texas->pots[texas->curr_poti].total_chip += chip;
    texas->pots[texas->curr_poti].player_chip[player_no] += chip;
    texas->players[player_no].gold -= chip;
    if(texas->players[player_no].gold == 0)
        texas->players[player_no].state = PLAYER_ACTION_ALLIN;
    else
        texas->players[player_no].state = PLAYER_ACTION_BET;

    texas_next_step(texas);
    
    return chip;
}

uint64_t texas_call(texas_t* texas, int player_no)
{
    uint64_t call_chip;

    if(!texas)
        return 0;
    if(player_no >= texas->player_num)
        return 0;
    if(player_no != texas->curr_player_no)
        return 0;
    if(texas->players[player_no].gold == 0)
        return 0;

    call_chip = texas_call_need_chip(texas, player_no);
    if(call_chip > texas->players[player_no].gold)
        call_chip = texas->players[player_no].gold;

    texas->pots[texas->curr_poti].total_chip += call_chip;
    texas->pots[texas->curr_poti].player_chip[player_no] += call_chip;
    texas->players[player_no].gold -= call_chip;
    if(texas->players[player_no].gold == 0)
        texas->players[player_no].state = PLAYER_ACTION_ALLIN;
    else
        texas->players[player_no].state = PLAYER_ACTION_CALL;

    texas_next_step(texas);

    return call_chip;
}

int texas_check(texas_t* texas, int player_no)
{
    uint64_t call_chip;

    if(!texas)
        return 0;
    if(player_no >= texas->player_num)
        return 0;
    if(player_no != texas->curr_player_no)
        return 0;

    if(texas->turn_max_chip > 0){
        call_chip = texas_call_need_chip(texas, player_no);
        if(call_chip > 0)
            return 0;
    }

    texas->players[player_no].state = PLAYER_ACTION_CHECK;
    texas_next_step(texas);

    return 1;
}

uint64_t texas_raiseto(texas_t* texas, int player_no, unsigned int chip)
{
    uint64_t call_chip;
    uint64_t raise_chip;

    if(!texas)
        return 0;
    if(player_no >= texas->player_num)
        return 0;
    if(player_no != texas->curr_player_no)
        return 0;
    if(texas->turn_max_chip == 0)
        return 0;
    if(chip > texas->players[player_no].gold)
        return 0;

    call_chip = texas_call_need_chip(texas, player_no);
    if(chip <= call_chip)
        return 0;
    raise_chip = chip - call_chip;
    if(raise_chip < texas->min_raise)
        return 0;

    if(raise_chip > texas->min_raise)
        texas->min_raise = raise_chip;
    texas->turn_max_chip += raise_chip;
    texas->pots[texas->curr_poti].total_chip += chip;
    texas->pots[texas->curr_poti].player_chip[player_no] += chip;
    texas->players[player_no].gold -= chip;
    if(texas->players[player_no].gold == 0)
        texas->players[player_no].state = PLAYER_ACTION_ALLIN;
    else
        texas->players[player_no].state = PLAYER_ACTION_RAISE;

    texas_next_step(texas);

    return chip;
}

uint64_t texas_raise(texas_t* texas, int player_no, unsigned int chip)
{
    uint64_t out_chip;

    if(!texas)
        return 0;
    if(player_no >= texas->player_num)
        return 0;
    if(player_no != texas->curr_player_no)
        return 0;
    if(texas->turn_max_chip == 0)
        return 0;
    if(chip < texas->min_raise)
        return 0;

    out_chip = texas_call_need_chip(texas, player_no);
    out_chip += chip;
    if(out_chip > texas->players[player_no].gold)
        return 0;

    if(chip > texas->min_raise)
        texas->min_raise = chip;
    texas->turn_max_chip += chip;
    texas->pots[texas->curr_poti].total_chip += out_chip;
    texas->pots[texas->curr_poti].player_chip[player_no] += out_chip;
    texas->players[player_no].gold -= out_chip;
    if(texas->players[player_no].gold == 0)
        texas->players[player_no].state = PLAYER_ACTION_ALLIN;
    else
        texas->players[player_no].state = PLAYER_ACTION_RAISE;

    texas_next_step(texas);

    return out_chip;
}

uint64_t texas_allin(texas_t* texas, int player_no)
{
    uint64_t chip;

    if(!texas)
        return 0;
    if(player_no >= texas->player_num)
        return 0;
    if(player_no != texas->curr_player_no)
        return 0;
    if(texas->players[player_no].gold == 0)
        return 0;

    chip = texas_call_need_chip(texas, player_no);
    if(chip < texas->players[player_no].gold)
        texas->turn_max_chip += texas->players[player_no].gold;

    chip = texas->players[player_no].gold;
    texas->pots[texas->curr_poti].total_chip += chip;
    texas->pots[texas->curr_poti].player_chip[player_no] += chip;
    texas->players[player_no].gold = 0;
    texas->players[player_no].state = PLAYER_ACTION_ALLIN;

    texas_next_step(texas);

    return chip;
}

int texas_logicvalue(card_t* card)
{
    if(!card)
        return 0;

    if(card->rank > cdRankK) return 0;

    return texas_table_rank[card->rank];
}

int texas_rankvalue(int logic_value)
{
    if(logic_value > 14 || logic_value < 0)
        return 0;
    return texas_table_logic[logic_value];
}