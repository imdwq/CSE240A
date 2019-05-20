//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <assert.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Ruiqi Zhang";
const char *studentID   = "A53279280";
const char *email       = "r7zhang@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

int ghistoryBits; // Number of bits used for Global History
int lhistoryBits; // Number of bits used for Local History
int pcIndexBits;  // Number of bits used for PC index
int bpType;       // Branch Prediction Type
int verbose;

//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//
//
//TODO: Add your own Branch Predictor data structures here
//

int globalHistory;
int maxGlobalHistory;

void addGlobalHistory(uint8_t outcome) {
  globalHistory = (globalHistory << 1) | outcome;
  globalHistory = globalHistory & maxGlobalHistory;
}

uint8_t** stateTable;
int stateTableRows;
int state; // 0, 1, 2, 3
int stateTableIndex;

void updateTable(uint8_t outcome) {
  stateTable[stateTableIndex][state] = outcome;
}

void addState(uint8_t outcome) {
  state = (state << 1) | outcome;
  state = state & 0x3;
}


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //
  globalHistory = 0;
  maxGlobalHistory = (1 << ghistoryBits) - 1;
  stateTableRows = 1 << ghistoryBits;

  stateTable = malloc(sizeof(uint8_t*) * stateTableRows);
  for(int i=0; i<stateTableRows; i++) {
    stateTable[i] = malloc(sizeof(uint8_t) * 4);
    for(int j=0; j<4; j++) {
      stateTable[i][j] = NOTTAKEN;
    }
  }

  state = 0;
}

uint8_t gshare_prediction(uint32_t pc) {
  stateTableIndex = (pc & maxGlobalHistory) ^ globalHistory;
  assert(stateTableIndex < stateTableRows);
  assert(state < 4);
  return stateTable[stateTableIndex][state];
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{
  //
  //TODO: Implement prediction scheme
  //

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return gshare_prediction(pc);
    case TOURNAMENT:
    case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//
void
train_predictor(uint32_t pc, uint8_t outcome)
{
  //
  //TODO: Implement Predictor training
  //
  addGlobalHistory(outcome);
  updateTable(outcome);
  addState(outcome);
  // printf("state: %x\n", state);
}
