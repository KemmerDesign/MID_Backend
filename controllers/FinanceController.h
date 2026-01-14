#pragma once
#include <drogon/HttpController.h>

using namespace drogon;

class FinanceController : public drogon::HttpController<FinanceController> {
    public:
        METHOD_LIST_BEGIN
        // Add your methods here. For example:
        // METHOD_ADD(FinanceController::getFinanceData, "/financeData", Get);
            ADD_METHOD_TO(FinanceController::calculateWacc, "/api/finance/wacc", Post);
        METHOD_LIST_END

        // Example method
        void calculateWacc(const HttpRequestPtr& req,
                           std::function<void (const HttpResponsePtr &)> &&callback);
        // ... (dentro de la clase FirebaseClient, sección public)

        
};