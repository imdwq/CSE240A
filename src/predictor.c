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

// gshare - global predictor
int globalHistory;
int maxGlobalHistory;

void addGlobalHistory(uint8_t outcome) {
  globalHistory = (globalHistory << 1) | outcome;
  globalHistory = globalHistory & maxGlobalHistory;
}

uint8_t* globalBHT;
int globalBHTRows;
int globalBHTIndex;
void updateGlobalBHT(uint8_t outcome) {
	assert(globalBHTIndex < globalBHTRows);
	if(outcome == NOTTAKEN) {
		if(globalBHT[globalBHTIndex] != SN) 
			globalBHT[globalBHTIndex] --;
	}
	else {
		if(globalBHT[globalBHTIndex] != ST)
			globalBHT[globalBHTIndex] ++;
	}
}


// tournament - local + global
int maxPC;
int localPHTRows;
int localPHTIndex;

int maxLocalHistory;
int localBHTRows;
int localBHTIndex;

uint8_t* selector;
uint8_t p1 = NOTTAKEN; // outcome of predictor 1
uint8_t p2 = NOTTAKEN; // outcome of predictor 2

void updateSelector(uint8_t outcome) {
	uint8_t p1Correct = (p1 == outcome);
	uint8_t p2Correct = (p1 == outcome);
	if(p1Correct > p2Correct) { // 1, 0
		if(selector[localPHTIndex] > 0) selector--;
	}
	else if(p1Correct < p2Correct) { // 0, 1
		if(selector[localPHTIndex] < 3) selector++;
	}
}

uint32_t* localPHT;
void updateLocalPHT(uint8_t outcome) {
  localPHT[localPHTIndex] = (localPHT[localPHTIndex] << 1) | outcome;
  localPHT[localPHTIndex] &= maxLocalHistory;
}

uint8_t* localBHT;
void updateLocalBHT(uint8_t outcome) {
	assert(localBHTIndex >=0 && localBHTIndex < localBHTRows);
	if(outcome == NOTTAKEN) {
		if(localBHT[localBHTIndex] != SN) 
			localBHT[localBHTIndex] --;
	}
	else {
		if(localBHT[localBHTIndex] != ST) 
			localBHT[localBHTIndex] ++;
	}
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
	// gshare -- global predictor
	globalHistory = 0;
	maxGlobalHistory = (1 << ghistoryBits) - 1;
	globalBHTRows = 1 << ghistoryBits;

	globalBHT = (uint8_t*)malloc(sizeof(uint8_t) * globalBHTRows);
	for(int i=0; i < globalBHTRows; i++) {
		globalBHT[i] = 1;
	}

	// tournament -- global + local
	maxPC = (1 << pcIndexBits) - 1;
	localPHTRows = 1 << pcIndexBits;

	maxLocalHistory = (1 << lhistoryBits) - 1;
	localBHTRows = 1 << lhistoryBits;

	localPHT = (uint32_t*)malloc(sizeof(uint32_t) * localPHTRows);
	for(int i=0; i < localPHTRows; i++) {
		localPHT[i] = 0;
	}

	localBHT = (uint8_t*)malloc(sizeof(uint8_t) * localBHTRows);
	for(int i=0; i < localBHTRows; i++) {
		localBHT[i] = 1;
	}

	selector = (uint8_t*)malloc(sizeof(uint8_t) * localPHTRows);
	for(int i=0; i < localPHTRows; i++) {
		selector[i] = 1;
	}
}


uint8_t gshare_prediction(uint32_t pc) {
	globalBHTIndex = (pc & maxGlobalHistory) ^ globalHistory;
	assert(globalBHTIndex >= 0 && globalBHTIndex < globalBHTRows);
	if(globalBHT[globalBHTIndex] == SN || globalBHT[globalBHTIndex] == WN) return NOTTAKEN;
	else return TAKEN;
}

uint8_t tournament_prediction(uint32_t pc) {
	localPHTIndex = pc & maxPC;
	// local predictor
	localBHTIndex = localPHT[localPHTIndex];
	p1 = localBHT[localBHTIndex];
	// global predictor
	globalBHTIndex = globalHistory & maxGlobalHistory;
	p2 = globalBHT[globalBHTIndex];

	// select
	if(selector[localPHTIndex] <= 1) { // local
		if(p1 == SN || p1 == WN) return NOTTAKEN;
		else return TAKEN;
	}
	else { // global
		if(p2 == SN || p2 == WN) return NOTTAKEN;
		else return TAKEN;
	}
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
    	return tournament_prediction(pc);
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
	switch(bpType) {
		case STATIC:
			break;
		case GSHARE:
			addGlobalHistory(outcome);
			updateGlobalBHT(outcome);
			break;
		case TOURNAMENT:
			addGlobalHistory(outcome);
			updateSelector(outcome);
			updateGlobalBHT(outcome);
			updateLocalPHT(outcome);
			updateLocalBHT(outcome);
			break;
		case CUSTOM:
			break;
		default:
			break;
	}
	
}
