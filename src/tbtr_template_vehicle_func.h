// template_vehicle_func.h
#ifndef TEMPLATE_VEHICLE_FUNC_H
#define TEMPLATE_VEHICLE_FUNC_H

#include "stdafx.h"
#include "window_gui.h"

#include "tbtr_template_vehicle.h"

//void DrawTemplateVehicle(TemplateVehicle*, int, const Rect&);


Train* VirtualTrainFromTemplateVehicle(TemplateVehicle* tv, StringID &err);

void BuildTemplateGuiList(GUITemplateList*, Scrollbar*, Owner, RailType);

Money CalculateOverallTemplateCost(const TemplateVehicle*);

void DrawTemplate(const TemplateVehicle*, int, int, int);

TemplateVehicle* TemplateVehicleFromVirtualTrain(Train *virt);

Train* DeleteVirtualTrain(Train*, Train *);

CommandCost CmdTemplateReplaceVehicle(Train*, bool, DoCommandFlag);

#ifdef _DEBUG
// for testing
void tbtr_debug_pat();
void tbtr_debug_pav();
void tbtr_debug_ptv(TemplateVehicle*);
void tbtr_debug_pvt(const Train*);
#endif

TemplateVehicle* GetTemplateVehicleByGroupID(GroupID);
bool ChainContainsVehicle(Train*, Train*);
Train* ChainContainsEngine(EngineID, Train*);
Train* DepotContainsEngine(TileIndex, EngineID, Train*);

int NumTrainsNeedTemplateReplacement(GroupID, TemplateVehicle*);

CommandCost TestBuyAllTemplateVehiclesInChain(TemplateVehicle *tv, TileIndex tile);

void CmdRefitTrainFromTemplate(Train *t, TemplateVehicle *tv, DoCommandFlag);
void BreakUpRemainders(Train *t);

bool TemplateVehicleContainsEngineOfRailtype(const TemplateVehicle*, RailType);

void TransferCargoForTrain(Train*, Train*);

void NeutralizeStatus(Train *t);

bool TrainMatchesTemplate(const Train *t, TemplateVehicle *tv);
bool TrainMatchesTemplateRefit(const Train *t, TemplateVehicle *tv);

#endif
