-- Migración 003: Módulo RRHH + Miembros de Proyecto
-- Ejecutar: PGPASSWORD=monarca_dev_2024 psql -h localhost -U monarca -d monarca_db -f MID_Backend/db/migrations/003_rrhh_and_project_members.sql

-- ── Empleados ─────────────────────────────────────────────────────────────────
-- Registro maestro de RRHH. Desacoplado de users: un empleado puede o no
-- tener acceso al sistema (user_id nullable). Un usuario del sistema puede
-- o no tener ficha de empleado.
CREATE TABLE IF NOT EXISTS public.employees (
    id                  UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    tenant_id           UUID        NOT NULL,
    user_id             UUID        REFERENCES public.users(id) ON DELETE SET NULL,
    nombres             TEXT        NOT NULL,
    apellidos           TEXT        NOT NULL,
    tipo_documento      TEXT        NOT NULL
                        CHECK (tipo_documento IN ('CC','CE','PA','TI','NIT')),
    numero_documento    TEXT        NOT NULL,
    email_personal      TEXT,
    email_corporativo   TEXT,
    telefono1           TEXT,
    telefono2           TEXT,
    fecha_nacimiento    DATE,
    direccion           TEXT,
    ciudad              TEXT        NOT NULL DEFAULT 'Bogotá',
    cargo               TEXT        NOT NULL,
    departamento        TEXT        NOT NULL
                        CHECK (departamento IN (
                            'comercial','proyectos','produccion',
                            'almacen','rrhh','gerencia','administrativo'
                        )),
    tipo_contrato       TEXT        NOT NULL
                        CHECK (tipo_contrato IN (
                            'indefinido','fijo','obra_labor',
                            'prestacion_servicios','aprendiz'
                        )),
    fecha_ingreso       DATE        NOT NULL,
    fecha_retiro        DATE,
    salario_base        NUMERIC(18,2),
    activo              BOOLEAN     NOT NULL DEFAULT TRUE,
    notas               TEXT,
    created_at          TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    updated_at          TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    CONSTRAINT employees_doc_tenant UNIQUE (numero_documento, tenant_id)
);

-- ── Miembros de Proyecto ──────────────────────────────────────────────────────
-- Relaciona empleados con proyectos según su rol en ese proyecto específico.
-- Un empleado puede tener distintos roles en distintos proyectos.
-- La relación comercial-proyecto ya existe en projects.commercial_id (user).
CREATE TABLE IF NOT EXISTS public.project_members (
    id          UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    project_id  UUID        NOT NULL REFERENCES public.projects(id) ON DELETE CASCADE,
    employee_id UUID        NOT NULL REFERENCES public.employees(id) ON DELETE CASCADE,
    rol_proyecto TEXT       NOT NULL
                CHECK (rol_proyecto IN (
                    'lider_proyectos',          -- Lidera la parte técnica / diseño
                    'desarrollador',            -- Diseñador / Ingeniero de producto
                    'coordinador_produccion',   -- Gestiona la fabricación en planta
                    'observador'                -- Solo lectura / seguimiento
                )),
    assigned_by UUID        REFERENCES public.users(id),
    activo      BOOLEAN     NOT NULL DEFAULT TRUE,
    assigned_at TIMESTAMPTZ NOT NULL DEFAULT NOW(),
    UNIQUE (project_id, employee_id, rol_proyecto)
);

-- ── Triggers updated_at ───────────────────────────────────────────────────────
DROP TRIGGER IF EXISTS trg_employees_updated_at ON public.employees;
CREATE TRIGGER trg_employees_updated_at
    BEFORE UPDATE ON public.employees FOR EACH ROW EXECUTE FUNCTION update_updated_at();

-- ── Índices ───────────────────────────────────────────────────────────────────
CREATE INDEX IF NOT EXISTS idx_employees_tenant      ON public.employees(tenant_id);
CREATE INDEX IF NOT EXISTS idx_employees_departamento ON public.employees(departamento);
CREATE INDEX IF NOT EXISTS idx_employees_user_id     ON public.employees(user_id);
CREATE INDEX IF NOT EXISTS idx_project_members_project  ON public.project_members(project_id);
CREATE INDEX IF NOT EXISTS idx_project_members_employee ON public.project_members(employee_id);
