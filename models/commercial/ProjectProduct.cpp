#include "ProjectProduct.h"
#include <stdexcept>

// ─── helpers ─────────────────────────────────────────────────────────────────

static auto nullable(const std::string& s) -> json {
    return s.empty() ? json(nullptr) : json(s);
}

static auto nullableInt(const std::string& s) -> json {
    if (s.empty()) return json(nullptr);
    try { return json(std::stoi(s)); } catch (...) { return json(nullptr); }
}

static auto nullableNum(const std::string& s) -> json {
    if (s.empty()) return json(nullptr);
    try { return json(std::stod(s)); } catch (...) { return json(nullptr); }
}

// ─── MaterialSuggestion ──────────────────────────────────────────────────────

json MaterialSuggestion::toJson() const {
    return {
        {"id",            id},
        {"tipo_material", tipo_material},
        {"descripcion",   nullable(descripcion)},
        {"sugerido_por",  nullable(sugerido_por)},
        {"created_at",    created_at}
    };
}

static MaterialSuggestion rowToMaterial(const PgResult& r, int row) {
    MaterialSuggestion m;
    m.id            = r.val(row, "id");
    m.tipo_material = r.val(row, "tipo_material");
    m.descripcion   = r.val(row, "descripcion");
    m.sugerido_por  = r.val(row, "sugerido_por");
    m.created_at    = r.val(row, "created_at");
    return m;
}

// ─── ProjectProduct ───────────────────────────────────────────────────────────

json ProjectProduct::toJson() const {
    json j = {
        {"id",                   id},
        {"project_id",           project_id},
        {"tenant_id",            tenant_id},
        {"catalog_id",           nullable(catalog_id)},
        {"nombre",               nombre},
        {"categoria",            categoria},
        {"alto_mm",              nullableInt(alto_mm)},
        {"ancho_mm",             nullableInt(ancho_mm)},
        {"profundidad_mm",       nullableInt(profundidad_mm)},
        {"canal_venta",          canal_venta},
        {"temporalidad",         temporalidad},
        {"temporalidad_meses",   nullableInt(temporalidad_meses)},
        {"cantidad",             nullableInt(cantidad)},
        {"presupuesto_estimado", nullableNum(presupuesto_estimado)},
        {"notas",                nullable(notas)},
        {"created_at",           created_at},
        {"updated_at",           updated_at},
        {"materiales",           json::array()}
    };
    for (const auto& m : materiales) j["materiales"].push_back(m.toJson());
    return j;
}

static ProjectProduct rowToProduct(const PgResult& r, int row) {
    ProjectProduct p;
    p.id                   = r.val(row, "id");
    p.project_id           = r.val(row, "project_id");
    p.tenant_id            = r.val(row, "tenant_id");
    p.catalog_id           = r.val(row, "catalog_id");
    p.nombre               = r.val(row, "nombre");
    p.categoria            = r.val(row, "categoria");
    p.alto_mm              = r.val(row, "alto_mm");
    p.ancho_mm             = r.val(row, "ancho_mm");
    p.profundidad_mm       = r.val(row, "profundidad_mm");
    p.canal_venta          = r.val(row, "canal_venta");
    p.temporalidad         = r.val(row, "temporalidad");
    p.temporalidad_meses   = r.val(row, "temporalidad_meses");
    p.cantidad             = r.val(row, "cantidad");
    p.presupuesto_estimado = r.val(row, "presupuesto_estimado");
    p.notas                = r.val(row, "notas");
    p.created_at           = r.val(row, "created_at");
    p.updated_at           = r.val(row, "updated_at");
    return p;
}

static std::vector<MaterialSuggestion> loadMaterials(const std::string& product_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.project_product_materials "
        "WHERE project_product_id=$1 ORDER BY created_at",
        {product_id});
    std::vector<MaterialSuggestion> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) out.push_back(rowToMaterial(r, i));
    return out;
}

// ─── CRUD ────────────────────────────────────────────────────────────────────

ProjectProduct ProjectProduct::create(const std::string& project_id,
                                       const std::string& tenant_id,
                                       const json& data) {
    auto str = [&](const char* key, const char* def = "") -> std::string {
        return (data.contains(key) && data[key].is_string()) ? data[key].get<std::string>() : def;
    };
    auto intStr = [&](const char* key) -> std::string {
        if (data.contains(key) && data[key].is_number_integer())
            return std::to_string(data[key].get<int>());
        return "";
    };
    auto numStr = [&](const char* key) -> std::string {
        if (data.contains(key) && data[key].is_number())
            return std::to_string(data[key].get<double>());
        return "";
    };

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.project_products "
        "(project_id, tenant_id, catalog_id, nombre, categoria, "
        " alto_mm, ancho_mm, profundidad_mm, canal_venta, temporalidad, "
        " temporalidad_meses, cantidad, presupuesto_estimado, notas) "
        "VALUES ($1,$2,NULLIF($3,'')::UUID,$4,$5,"
        "        NULLIF($6,'')::INTEGER, NULLIF($7,'')::INTEGER, NULLIF($8,'')::INTEGER,"
        "        $9, $10,"
        "        NULLIF($11,'')::INTEGER, COALESCE(NULLIF($12,'')::INTEGER, 1),"
        "        NULLIF($13,'')::NUMERIC, NULLIF($14,'')) "
        "RETURNING *",
        {project_id, tenant_id,
         str("catalog_id"),
         str("nombre"), str("categoria"),
         intStr("alto_mm"), intStr("ancho_mm"), intStr("profundidad_mm"),
         str("canal_venta"), str("temporalidad"),
         intStr("temporalidad_meses"), intStr("cantidad"),
         numStr("presupuesto_estimado"), str("notas")}
    );

    if (!r.ok() || r.rows() == 0)
        throw std::runtime_error(std::string("ProjectProduct::create: ") + r.errMsg());
    return rowToProduct(r, 0);
}

std::vector<ProjectProduct> ProjectProduct::findByProject(const std::string& project_id,
                                                           const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.project_products "
        "WHERE project_id=$1 AND tenant_id=$2 ORDER BY created_at",
        {project_id, tenant_id});

    std::vector<ProjectProduct> out;
    if (!r.ok()) return out;
    for (int i = 0; i < r.rows(); ++i) {
        auto p = rowToProduct(r, i);
        p.materiales = loadMaterials(p.id);
        out.push_back(std::move(p));
    }
    return out;
}

std::optional<ProjectProduct> ProjectProduct::findById(const std::string& id,
                                                        const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "SELECT * FROM public.project_products WHERE id=$1 AND tenant_id=$2 LIMIT 1",
        {id, tenant_id});
    if (!r.ok() || r.rows() == 0) return std::nullopt;
    auto p = rowToProduct(r, 0);
    p.materiales = loadMaterials(p.id);
    return p;
}

bool ProjectProduct::remove(const std::string& id, const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "DELETE FROM public.project_products WHERE id=$1 AND tenant_id=$2 RETURNING id",
        {id, tenant_id});
    return r.cmdOk() || (r.ok() && r.rows() > 0);
}

// ─── Materials ────────────────────────────────────────────────────────────────

MaterialSuggestion ProjectProduct::addMaterial(const std::string& project_product_id,
                                                const std::string& tenant_id,
                                                const json& data,
                                                const std::string& sugerido_por) {
    auto str = [&](const char* key, const char* def = "") -> std::string {
        return (data.contains(key) && data[key].is_string()) ? data[key].get<std::string>() : def;
    };

    auto r = PgConnection::getInstance().exec(
        "INSERT INTO public.project_product_materials "
        "(project_product_id, tenant_id, tipo_material, descripcion, sugerido_por) "
        "VALUES ($1, $2, $3, NULLIF($4,''), NULLIF($5,'')::UUID) "
        "RETURNING *",
        {project_product_id, tenant_id,
         str("tipo_material"), str("descripcion"), sugerido_por}
    );

    if (!r.ok() || r.rows() == 0)
        throw std::runtime_error(std::string("ProjectProduct::addMaterial: ") + r.errMsg());
    return rowToMaterial(r, 0);
}

bool ProjectProduct::removeMaterial(const std::string& material_id,
                                     const std::string& project_product_id,
                                     const std::string& tenant_id) {
    auto r = PgConnection::getInstance().exec(
        "DELETE FROM public.project_product_materials "
        "WHERE id=$1 AND project_product_id=$2 AND tenant_id=$3 RETURNING id",
        {material_id, project_product_id, tenant_id});
    return r.cmdOk() || (r.ok() && r.rows() > 0);
}
