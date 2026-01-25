#pragma once
#include <drogon/HttpController.h>
#include "../../models/rawMaterial/MetalMaterial.h"

using namespace drogon;


class MetalMaterialController : public drogon::HttpController<MetalMaterialController> {
    public:
        METHOD_LIST_BEGIN
        // Add your methods here. For example:
        // METHOD_ADD(MetalMaterialController::getMetalData, "/metalData", Get);
        // Add methods as needed
            ADD_METHOD_TO(MetalMaterialController::addPipeToStock, "/api/stock/pipeEntryStock", Post);
        METHOD_LIST_END
            void addPipeToStock(const HttpRequestPtr& req,
                               std::function<void (const HttpResponsePtr &)> &&callback);

};