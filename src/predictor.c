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

void updateTwoBitsHistoryTable(uint8_t *historyTable, int index, uint8_t outcome){
	if(outcome == NOTTAKEN) {
		if(historyTable[index] != SN) 
			historyTable[index]--;
	}
	else {
		if(historyTable[index] != ST)
			historyTable[index]++;
	}
}

// gshare - global predictor
int globalHistory;
int maxGlobalHistory;

void addGlobalHistory(uint8_t outcome) {
  globalHistory = (globalHistory << 1) | outcome;
  globalHistory &= maxGlobalHistory;
}

uint8_t* globalBHT;
int globalBHTRows;
int globalBHTIndex;
void updateGlobalBHT(uint8_t outcome) {
	assert(globalBHTIndex < globalBHTRows);
	updateTwoBitsHistoryTable(globalBHT, globalBHTIndex, outcome);
}

// tournament - local + global
int maxPC;
int localPHTRows;
int localPHTIndex;

int maxLocalHistory;
int localBHTRows;
int localBHTIndex;

uint8_t p1 = NOTTAKEN; // outcome of predictor 1
uint8_t p2 = NOTTAKEN; // outcome of predictor 2

int p1CorrectCount = 0;
int p2CorrectCount = 0;

uint8_t* selector;
void updateSelector(uint8_t outcome) {
	uint8_t p1Correct = (p1 == outcome);
	uint8_t p2Correct = (p2 == outcome);
	if(p1Correct > p2Correct) // 1, 0
		updateTwoBitsHistoryTable(selector, globalBHTIndex, TAKEN);
	else if(p1Correct < p2Correct) { // 0, 1
		updateTwoBitsHistoryTable(selector, globalBHTIndex, NOTTAKEN);
	}
	p1CorrectCount += p1Correct;
	p2CorrectCount += p2Correct;
}

uint32_t* localPHT;
void updateLocalPHT(uint8_t outcome) {
  localPHT[localPHTIndex] = (localPHT[localPHTIndex] << 1) | outcome;
  localPHT[localPHTIndex] &= maxLocalHistory;
}

uint8_t* localBHT;
void updateLocalBHT(uint8_t outcome) {
	assert(localBHTIndex >= 0 && localBHTIndex < localBHTRows);
	updateTwoBitsHistoryTable(localBHT, localBHTIndex, outcome);
}

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

void init_global() {
	globalHistory = 0;
	maxGlobalHistory = (1 << ghistoryBits) - 1;
	globalBHTRows = 1 << ghistoryBits;

	globalBHT = (uint8_t*)malloc(sizeof(uint8_t) * globalBHTRows);
	for(int i=0; i < globalBHTRows; i++) {
		globalBHT[i] = WN;
	}
}

void init_tournament() {
	init_global();

	maxPC = (1 << pcIndexBits) - 1;
	localPHTRows = 1 << pcIndexBits;

	maxLocalHistory = (1 << lhistoryBits) - 1;
	localBHTRows = 1 << lhistoryBits;

	localPHT = (uint32_t*)malloc(sizeof(uint32_t) * localPHTRows);
	for(int i=0; i < localPHTRows; i++) localPHT[i] = 0;

	localBHT = (uint8_t*)malloc(sizeof(uint8_t) * localBHTRows);
	for(int i=0; i < localBHTRows; i++) localBHT[i] = WN;

	selector = (uint8_t*)malloc(sizeof(uint8_t) * globalBHTRows);
	for(int i=0; i < globalBHTRows; i++) selector[i] = WN;
}

// storage
int8_t predictorWeight[PCSIZE][MAXHISTORYSIZE + 1];
uint8_t globalHistoryNN[MAXHISTORYSIZE];  // Actually store bits
int historySize;
int threshold;
int pcBias;

// temp global
uint8_t prediction;
int out;
int nnPCIndex;
int nnPCMask;

void init_custom(){
	nnPCIndex = PCBITS;
	nnPCMask = ((1 << nnPCIndex) - 1) << pcBias;
	for(int i = 0; i < PCSIZE; ++i){
		for(int j = 0; j < historySize + 1; ++j)
			predictorWeight[i][j] = 0;
	}
	for(int i = 0; i < historySize; ++i)
		globalHistoryNN[i] = NOTTAKEN;
}

uint8_t custom_prediction(uint32_t pc){
	int pcIndex = (pc & nnPCMask) >> pcBias;
	// out = bias[pcIndex];
	out = predictorWeight[pcIndex][0];
	for(int i = 0; i < historySize; ++i){
		int history = globalHistoryNN[i]? 1: -1;
		out += predictorWeight[pcIndex][i+1] * history;
	}
	prediction = out > 0;
	return prediction; 
}

void custom_train(uint32_t pc, uint8_t outcome){
	int pcIndex = (pc & nnPCMask) >> pcBias;
	if(prediction != outcome || (out < threshold && out > -threshold)){
		for(int i = 0; i < historySize; ++i){
			int history = globalHistoryNN[i]? 1: -1;
			if(outcome == globalHistoryNN[i] && predictorWeight[pcIndex][i+1] < 127)
					predictorWeight[pcIndex][i+1]++;
			else if(outcome != globalHistoryNN[i] && predictorWeight[pcIndex][i+1] > -128)
					predictorWeight[pcIndex][i+1]--;
		}

		if(predictorWeight[pcIndex][0] < 127 && outcome)
			predictorWeight[pcIndex][0]++;
		else if(predictorWeight[pcIndex][0] > -128 && !outcome)
			predictorWeight[pcIndex][0]--;

	}
	for(int i = historySize - 1; i > 0; --i)
		globalHistoryNN[i] = globalHistoryNN[i-1];
	globalHistoryNN[0] = outcome;
}

void init_customTournament(){
	ghistoryBits = 12;
	init_global();
	init_custom();
	selector = (uint8_t*)malloc(sizeof(uint8_t) * globalBHTRows);
	for(int i=0; i < globalBHTRows; i++) selector[i] = WN;
}

// Initialize the predictor
//
void
init_predictor()
{
  //
  //TODO: Initialize Branch Predictor Data Structures
  //
	switch(bpType) {
		case STATIC:
			break;
		case GSHARE:
			init_global();
			break;
		case TOURNAMENT:
			init_tournament();
			break;
		case CUSTOM:
			init_custom();
			break;
		case CUSTOMTOUR:
			init_customTournament();
		default:
			break;
	}
	
}

uint8_t two_bits_prediction(uint8_t *branchHistoryTable, int index){
	switch(branchHistoryTable[index]){
		case SN: ;
		case WN: return NOTTAKEN;
		case WT: ;
		case ST: return TAKEN;
		default: assert(0); return -1;
	}
}

uint8_t gshare_prediction(uint32_t pc) {
	globalBHTIndex = (pc & maxGlobalHistory) ^ globalHistory;
	assert(globalBHTIndex >= 0 && globalBHTIndex < globalBHTRows);
	return two_bits_prediction(globalBHT, globalBHTIndex);
}

uint8_t global_prediction() {
	globalBHTIndex = globalHistory & maxGlobalHistory;
	return two_bits_prediction(globalBHT, globalBHTIndex);
}

uint8_t local_prediction(uint32_t pc) {
	localPHTIndex = pc & maxPC;
	localBHTIndex = localPHT[localPHTIndex];
	return two_bits_prediction(localBHT, localBHTIndex);
}

uint8_t tournament_prediction(uint32_t pc) {
	// local predictor
	p1 = local_prediction(pc);
	// global predictor
	p2 = global_prediction();
	// select global if not taken else local
	return two_bits_prediction(selector, globalBHTIndex)? p1: p2;
}

uint8_t tournament_predictionNNwithGshare(uint32_t pc){
	// local predictor
	p1 = custom_prediction(pc);
	// global predictor
	p2 = global_prediction();
	// select global if not taken else local
	return two_bits_prediction(selector, globalBHTIndex)? p1: p2;
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
			return custom_prediction(pc);
		case CUSTOMTOUR:
			return tournament_predictionNNwithGshare(pc);
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
			custom_train(pc, outcome);
			break;
		case CUSTOMTOUR:
			addGlobalHistory(outcome);
			updateSelector(outcome);
			updateGlobalBHT(outcome);
			custom_train(pc, outcome);
		default:
			break;
	}
	
}
