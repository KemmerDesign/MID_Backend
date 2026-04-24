-- Migración 002: Módulo Comercial
-- Ejecutar: PGPASSWORD=monarca_dev_2024 psql -h localhost -U monarca -d monarca_db -f MID_Backend/db/migrations/002_commercial_module.sql

-- ── Clientes ──────────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS public.clients (
    id                  UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    tenant_id           UUID        NOT NULL,
    nombre              TEXT        NOT NULL,
    nombre_comercial    TEXT,
    nit                 TEXT        NOT NULL,
    direccion_principal TEXT        NOT NULL,
    ciudad_principal    TEXT        NOT NULL DEFAULT 'Bogotá',
    correo_facturacion  TEXT,
    sitio_web           TEXT,
    sector              TEXT,
    activo              BOOLEAN     NOT NULL DEFAULT TRUE,
    notas               TEXT,
    created_at          TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at          TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT clients_nit_tenant UNIQUE (nit, tenant_id)
);

-- Sedes / ubicaciones del cliente (cadena de supermercados, múltiples bodegas, etc.)
CREATE TABLE IF NOT EXISTS public.client_addresses (
    id           UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    client_id    UUID        NOT NULL REFERENCES public.clients(id) ON DELETE CASCADE,
    etiqueta     TEXT        NOT NULL,   -- "Sede Principal", "Bodega Norte", etc.
    direccion    TEXT        NOT NULL,
    ciudad       TEXT        NOT NULL,
    departamento TEXT,
    pais         TEXT        NOT NULL DEFAULT 'Colombia',
    es_principal BOOLEAN     NOT NULL DEFAULT FALSE,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Personas de contacto del cliente
CREATE TABLE IF NOT EXISTS public.client_contacts (
    id           UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    client_id    UUID        NOT NULL REFERENCES public.clients(id) ON DELETE CASCADE,
    nombre       TEXT        NOT NULL,
    cargo        TEXT,
    email        TEXT,
    telefono1    TEXT,
    telefono2    TEXT,
    es_principal BOOLEAN     NOT NULL DEFAULT FALSE,
    created_at   TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- Registro de visitas comerciales
CREATE TABLE IF NOT EXISTS public.client_visits (
    id             UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    client_id      UUID        NOT NULL REFERENCES public.clients(id),
    commercial_id  UUID        NOT NULL REFERENCES public.users(id),
    tenant_id      UUID        NOT NULL,
    fecha_visita   DATE        NOT NULL,
    tipo           TEXT        NOT NULL CHECK (tipo IN ('presencial', 'virtual', 'telefonica')),
    resumen        TEXT        NOT NULL,
    compromisos    TEXT,
    created_at     TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- ── Proyectos ─────────────────────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS public.projects (
    id                  UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    tenant_id           UUID         NOT NULL,
    client_id           UUID         NOT NULL REFERENCES public.clients(id),
    commercial_id       UUID         NOT NULL REFERENCES public.users(id),
    nombre              TEXT         NOT NULL,
    descripcion         TEXT,
    estado              TEXT         NOT NULL DEFAULT 'borrador'
                        CHECK (estado IN ('borrador','en_revision','aprobado','en_produccion','entregado','cancelado')),
    fecha_prometida     DATE,
    fecha_entrega_real  DATE,
    presupuesto_cliente NUMERIC(18,2),
    notas               TEXT,
    created_at          TIMESTAMPTZ  NOT NULL DEFAULT NOW(),
    updated_at          TIMESTAMPTZ  NOT NULL DEFAULT NOW()
);

-- Cadena de aprobación por departamento
CREATE TABLE IF NOT EXISTS public.project_approvals (
    id               UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    project_id       UUID        NOT NULL REFERENCES public.projects(id) ON DELETE CASCADE,
    departamento     TEXT        NOT NULL CHECK (departamento IN ('produccion','proyectos','almacen','gerencia')),
    approver_id      UUID        REFERENCES public.users(id),
    estado           TEXT        NOT NULL DEFAULT 'pendiente'
                     CHECK (estado IN ('pendiente','aprobado','rechazado')),
    comentarios      TEXT,
    fecha_resolucion TIMESTAMPTZ,
    created_at       TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (project_id, departamento)
);

-- ── Solicitudes de Producto ───────────────────────────────────────────────────
-- El comercial describe la necesidad; producción y desarrollo determinan materialidades finales.
CREATE TABLE IF NOT EXISTS public.product_requests (
    id                    UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    project_id            UUID        NOT NULL REFERENCES public.projects(id) ON DELETE CASCADE,
    tenant_id             UUID        NOT NULL,
    commercial_id         UUID        NOT NULL REFERENCES public.users(id),
    nombre_producto       TEXT        NOT NULL,
    descripcion_necesidad TEXT        NOT NULL,
    cantidad_estimada     INTEGER     NOT NULL CHECK (cantidad_estimada > 0),
    presupuesto_estimado  NUMERIC(18,2),
    unidad_medida         TEXT        NOT NULL DEFAULT 'unidad',
    estado                TEXT        NOT NULL DEFAULT 'pendiente'
                          CHECK (estado IN ('pendiente','en_revision','aprobado','rechazado')),
    notas_adicionales     TEXT,
    created_at            TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at            TIMESTAMPTZ NOT NULL DEFAULT NOW()
);

-- ── Triggers updated_at ───────────────────────────────────────────────────────
CREATE OR REPLACE FUNCTION update_updated_at()
RETURNS TRIGGER LANGUAGE plpgsql AS $$
BEGIN NEW.updated_at = NOW(); RETURN NEW; END;
$$;

DROP TRIGGER IF EXISTS trg_clients_updated_at        ON public.clients;
DROP TRIGGER IF EXISTS trg_projects_updated_at       ON public.projects;
DROP TRIGGER IF EXISTS trg_product_requests_updated_at ON public.product_requests;

CREATE TRIGGER trg_clients_updated_at
    BEFORE UPDATE ON public.clients FOR EACH ROW EXECUTE FUNCTION update_updated_at();
CREATE TRIGGER trg_projects_updated_at
    BEFORE UPDATE ON public.projects FOR EACH ROW EXECUTE FUNCTION update_updated_at();
CREATE TRIGGER trg_product_requests_updated_at
    BEFORE UPDATE ON public.product_requests FOR EACH ROW EXECUTE FUNCTION update_updated_at();

-- ── Índices ───────────────────────────────────────────────────────────────────
CREATE INDEX IF NOT EXISTS idx_clients_tenant          ON public.clients(tenant_id);
CREATE INDEX IF NOT EXISTS idx_clients_nit             ON public.clients(nit);
CREATE INDEX IF NOT EXISTS idx_projects_tenant         ON public.projects(tenant_id);
CREATE INDEX IF NOT EXISTS idx_projects_client         ON public.projects(client_id);
CREATE INDEX IF NOT EXISTS idx_projects_commercial     ON public.projects(commercial_id);
CREATE INDEX IF NOT EXISTS idx_product_requests_project ON public.product_requests(project_id);
CREATE INDEX IF NOT EXISTS idx_visits_client           ON public.client_visits(client_id);
CREATE INDEX IF NOT EXISTS idx_visits_tenant           ON public.client_visits(tenant_id);
