-- Migración 004: reemplaza product_requests por un modelo de productos real
-- ─────────────────────────────────────────────────────────────────────────────

DROP TABLE IF EXISTS public.product_requests CASCADE;

-- Catálogo de tipos de producto reutilizables por tenant
CREATE TABLE public.product_catalog (
    id          UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    tenant_id   UUID        NOT NULL,
    nombre      TEXT        NOT NULL,
    categoria   TEXT        NOT NULL,
    descripcion TEXT,
    activo      BOOLEAN     NOT NULL DEFAULT true,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT uq_product_catalog_nombre_tenant UNIQUE (nombre, tenant_id)
);

CREATE INDEX idx_product_catalog_tenant ON public.product_catalog (tenant_id);

-- Productos concretos ligados a un proyecto
CREATE TABLE public.project_products (
    id                  UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    project_id          UUID        NOT NULL REFERENCES public.projects(id) ON DELETE CASCADE,
    tenant_id           UUID        NOT NULL,
    catalog_id          UUID        REFERENCES public.product_catalog(id) ON DELETE SET NULL,
    nombre              TEXT        NOT NULL,
    categoria           TEXT        NOT NULL,
    alto_mm             INTEGER,
    ancho_mm            INTEGER,
    profundidad_mm      INTEGER,
    canal_venta         TEXT        NOT NULL
                            CHECK (canal_venta IN (
                                'supermercado','farmacia','ferreteria',
                                'cadena_retail','tienda_especializada',
                                'pdv_propio','otro'
                            )),
    temporalidad        TEXT        NOT NULL
                            CHECK (temporalidad IN (
                                'permanente','estacional','campana','temporal'
                            )),
    temporalidad_meses  INTEGER,
    cantidad            INTEGER     NOT NULL DEFAULT 1 CHECK (cantidad > 0),
    presupuesto_estimado NUMERIC(14,2),
    notas               TEXT,
    created_at          TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at          TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX idx_project_products_project ON public.project_products (project_id);
CREATE INDEX idx_project_products_tenant  ON public.project_products (tenant_id);

-- Sugerencias de materialidad para cada producto del proyecto
CREATE TABLE public.project_product_materials (
    id                  UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    project_product_id  UUID        NOT NULL
                            REFERENCES public.project_products(id) ON DELETE CASCADE,
    tenant_id           UUID        NOT NULL,
    tipo_material       TEXT        NOT NULL
                            CHECK (tipo_material IN (
                                'metal','madera','aglomerado','polimero',
                                'impresion','iluminacion','sensores',
                                'pantalla_led','iot','vidrio','acrilico','otro'
                            )),
    descripcion         TEXT,
    sugerido_por        UUID        REFERENCES public.users(id) ON DELETE SET NULL,
    created_at          TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX idx_ppm_product ON public.project_product_materials (project_product_id);

-- Triggers updated_at
CREATE OR REPLACE FUNCTION public.set_updated_at()
RETURNS TRIGGER LANGUAGE plpgsql AS $$
BEGIN NEW.updated_at = now(); RETURN NEW; END;
$$;

CREATE TRIGGER trg_product_catalog_updated_at
    BEFORE UPDATE ON public.product_catalog
    FOR EACH ROW EXECUTE FUNCTION public.set_updated_at();

CREATE TRIGGER trg_project_products_updated_at
    BEFORE UPDATE ON public.project_products
    FOR EACH ROW EXECUTE FUNCTION public.set_updated_at();
