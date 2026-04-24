-- Migración 005: módulo RRHH extendido
-- Añade: tipo_trabajador/contratista en employees, catálogo de roles,
-- historial laboral, dependientes, ausencias, vacaciones y disciplinario.
-- ─────────────────────────────────────────────────────────────────────────────

-- ── 1. Extender employees para contratistas ───────────────────────────────────
ALTER TABLE public.employees
    ADD COLUMN IF NOT EXISTS tipo_trabajador TEXT NOT NULL DEFAULT 'empleado_directo'
        CHECK (tipo_trabajador IN ('empleado_directo','contratista')),
    ADD COLUMN IF NOT EXISTS valor_honorarios    NUMERIC(14,2),
    ADD COLUMN IF NOT EXISTS periodicidad_pago   TEXT
        CHECK (periodicidad_pago IN ('mensual','quincenal','semanal','por_entregable')),
    ADD COLUMN IF NOT EXISTS objeto_contrato     TEXT,
    ADD COLUMN IF NOT EXISTS fecha_fin_contrato  DATE;

-- ── 2. Catálogo de roles/cargos por tenant ────────────────────────────────────
CREATE TABLE IF NOT EXISTS public.roles (
    id               UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    tenant_id        UUID        NOT NULL,
    nombre           TEXT        NOT NULL,
    descripcion      TEXT,
    departamento     TEXT        NOT NULL
        CHECK (departamento IN (
            'comercial','proyectos','produccion','almacen',
            'rrhh','gerencia','administrativo','financiero','tecnologia','otro'
        )),
    nivel_jerarquico INTEGER     NOT NULL DEFAULT 1
        CHECK (nivel_jerarquico BETWEEN 1 AND 10),
    activo           BOOLEAN     NOT NULL DEFAULT true,
    created_at       TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at       TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT uq_roles_nombre_dept_tenant UNIQUE (nombre, departamento, tenant_id)
);

CREATE INDEX IF NOT EXISTS idx_roles_tenant ON public.roles (tenant_id);

-- ── 3. Historial laboral (cargos, departamentos, salarios con vigencias) ───────
-- Cada fila = un período de tiempo en un puesto.
-- fecha_fin NULL = posición actual.
-- Al hacer un cambio de puesto se cierra el registro activo y se inserta uno nuevo.
CREATE TABLE IF NOT EXISTS public.employee_positions (
    id            UUID   PRIMARY KEY DEFAULT gen_random_uuid(),
    employee_id   UUID   NOT NULL REFERENCES public.employees(id) ON DELETE CASCADE,
    tenant_id     UUID   NOT NULL,
    rol_id        UUID   REFERENCES public.roles(id) ON DELETE SET NULL,
    cargo         TEXT   NOT NULL,
    departamento  TEXT   NOT NULL,
    salario_base  NUMERIC(14,2) NOT NULL,
    fecha_inicio  DATE   NOT NULL,
    fecha_fin     DATE,
    motivo        TEXT   NOT NULL
        CHECK (motivo IN (
            'contratacion','ascenso','traslado',
            'ajuste_salarial','reclasificacion','cambio_departamento','otro'
        )),
    notas         TEXT,
    registrado_por UUID  REFERENCES public.users(id) ON DELETE SET NULL,
    created_at    TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT chk_ep_dates CHECK (fecha_fin IS NULL OR fecha_fin > fecha_inicio)
);

CREATE INDEX IF NOT EXISTS idx_emp_positions_employee ON public.employee_positions (employee_id);
CREATE INDEX IF NOT EXISTS idx_emp_positions_tenant   ON public.employee_positions (tenant_id);
CREATE INDEX IF NOT EXISTS idx_emp_positions_active   ON public.employee_positions (employee_id)
    WHERE fecha_fin IS NULL;

-- Poblar historial con el estado actual de los empleados existentes
INSERT INTO public.employee_positions
    (employee_id, tenant_id, cargo, departamento, salario_base, fecha_inicio, motivo)
SELECT
    id,
    tenant_id,
    cargo,
    departamento,
    COALESCE(salario_base, 0),
    COALESCE(fecha_ingreso::DATE, CURRENT_DATE),
    'contratacion'
FROM public.employees
WHERE NOT EXISTS (
    SELECT 1 FROM public.employee_positions ep WHERE ep.employee_id = employees.id
);

-- ── 4. Dependientes (hijos, cónyuge, etc.) ────────────────────────────────────
-- Registrar hijos permite planear actividades (Día de los Niños, etc.),
-- calcular licencias de maternidad/paternidad y beneficios de nómina.
CREATE TABLE IF NOT EXISTS public.employee_dependents (
    id                UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    employee_id       UUID        NOT NULL REFERENCES public.employees(id) ON DELETE CASCADE,
    tenant_id         UUID        NOT NULL,
    nombres           TEXT        NOT NULL,
    apellidos         TEXT        NOT NULL,
    fecha_nacimiento  DATE,
    parentesco        TEXT        NOT NULL
        CHECK (parentesco IN (
            'hijo','hija','conyuge','companero_permanente','otro'
        )),
    vive_con_empleado BOOLEAN     NOT NULL DEFAULT true,
    activo            BOOLEAN     NOT NULL DEFAULT true,
    notas             TEXT,
    created_at        TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at        TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_emp_dependents_employee ON public.employee_dependents (employee_id);

-- ── 5. Ausencias laborales (incapacidades, licencias, permisos) ───────────────
-- dias_calendario se calcula automáticamente; remunerado determina
-- si afecta la nómina del período.
CREATE TABLE IF NOT EXISTS public.employee_absences (
    id                UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    employee_id       UUID        NOT NULL REFERENCES public.employees(id) ON DELETE CASCADE,
    tenant_id         UUID        NOT NULL,
    tipo              TEXT        NOT NULL
        CHECK (tipo IN (
            'incapacidad_medica',
            'accidente_trabajo',
            'licencia_maternidad',
            'licencia_paternidad',
            'licencia_luto',
            'permiso_remunerado',
            'permiso_no_remunerado',
            'calamidad_domestica',
            'suspension_disciplinaria',
            'otro'
        )),
    fecha_inicio      DATE        NOT NULL,
    fecha_fin         DATE        NOT NULL,
    dias_calendario   INTEGER     GENERATED ALWAYS AS
                          (fecha_fin - fecha_inicio + 1) STORED,
    remunerado        BOOLEAN     NOT NULL DEFAULT true,
    descripcion       TEXT,
    documento_soporte TEXT,
    estado            TEXT        NOT NULL DEFAULT 'solicitado'
        CHECK (estado IN (
            'solicitado','aprobado','rechazado','en_curso','completado','cancelado'
        )),
    aprobado_por      UUID        REFERENCES public.users(id) ON DELETE SET NULL,
    created_by        UUID        REFERENCES public.users(id) ON DELETE SET NULL,
    created_at        TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at        TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT chk_absence_dates CHECK (fecha_fin >= fecha_inicio)
);

CREATE INDEX IF NOT EXISTS idx_emp_absences_employee ON public.employee_absences (employee_id);
CREATE INDEX IF NOT EXISTS idx_emp_absences_tipo     ON public.employee_absences (tipo);

-- ── 6. Vacaciones (saldo y disfrute por período anual) ────────────────────────
-- Un registro por empleado por año. Permite saber saldo disponible,
-- días tomados y estado de la solicitud.
CREATE TABLE IF NOT EXISTS public.employee_vacations (
    id               UUID         PRIMARY KEY DEFAULT gen_random_uuid(),
    employee_id      UUID         NOT NULL REFERENCES public.employees(id) ON DELETE CASCADE,
    tenant_id        UUID         NOT NULL,
    periodo_year     INTEGER      NOT NULL,
    dias_disponibles NUMERIC(5,1) NOT NULL DEFAULT 15,
    dias_tomados     NUMERIC(5,1) NOT NULL DEFAULT 0
        CHECK (dias_tomados >= 0),
    fecha_inicio     DATE,
    fecha_fin        DATE,
    estado           TEXT         NOT NULL DEFAULT 'pendiente'
        CHECK (estado IN (
            'pendiente','solicitado','aprobado','rechazado','completado','pago_en_dinero'
        )),
    aprobado_por     UUID         REFERENCES public.users(id) ON DELETE SET NULL,
    notas            TEXT,
    created_at       TIMESTAMPTZ  NOT NULL DEFAULT now(),
    updated_at       TIMESTAMPTZ  NOT NULL DEFAULT now(),
    CONSTRAINT uq_vacation_employee_year UNIQUE (employee_id, periodo_year),
    CONSTRAINT chk_vac_days CHECK (dias_tomados <= dias_disponibles),
    CONSTRAINT chk_vac_dates CHECK (
        fecha_fin IS NULL OR fecha_inicio IS NULL OR fecha_fin >= fecha_inicio
    )
);

CREATE INDEX IF NOT EXISTS idx_emp_vacations_employee ON public.employee_vacations (employee_id);

-- ── 7. Disciplinario (llamados de atención, suspensiones, descargos) ──────────
-- El historial disciplinario es soporte legal para procesos de desvinculación.
-- Estado 'firmado_empleado' requiere fecha_firma.
CREATE TABLE IF NOT EXISTS public.employee_disciplinary (
    id                    UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    employee_id           UUID        NOT NULL REFERENCES public.employees(id) ON DELETE CASCADE,
    tenant_id             UUID        NOT NULL,
    tipo                  TEXT        NOT NULL
        CHECK (tipo IN (
            'llamado_verbal','llamado_escrito',
            'acta_compromiso','suspension','descargo','otro'
        )),
    fecha                 DATE        NOT NULL,
    motivo                TEXT        NOT NULL,
    descripcion           TEXT,
    estado                TEXT        NOT NULL DEFAULT 'emitido'
        CHECK (estado IN (
            'emitido','notificado','firmado_empleado','en_apelacion','cerrado'
        )),
    firmado_por_empleado  BOOLEAN     NOT NULL DEFAULT false,
    fecha_firma           DATE,
    created_by            UUID        REFERENCES public.users(id) ON DELETE SET NULL,
    created_at            TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at            TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT chk_disc_firma CHECK (
        firmado_por_empleado = false OR fecha_firma IS NOT NULL
    )
);

CREATE INDEX IF NOT EXISTS idx_emp_disciplinary_employee ON public.employee_disciplinary (employee_id);

-- Triggers updated_at
CREATE TRIGGER trg_roles_updated_at
    BEFORE UPDATE ON public.roles
    FOR EACH ROW EXECUTE FUNCTION public.set_updated_at();

CREATE TRIGGER trg_emp_dependents_updated_at
    BEFORE UPDATE ON public.employee_dependents
    FOR EACH ROW EXECUTE FUNCTION public.set_updated_at();

CREATE TRIGGER trg_emp_absences_updated_at
    BEFORE UPDATE ON public.employee_absences
    FOR EACH ROW EXECUTE FUNCTION public.set_updated_at();

CREATE TRIGGER trg_emp_vacations_updated_at
    BEFORE UPDATE ON public.employee_vacations
    FOR EACH ROW EXECUTE FUNCTION public.set_updated_at();

CREATE TRIGGER trg_emp_disciplinary_updated_at
    BEFORE UPDATE ON public.employee_disciplinary
    FOR EACH ROW EXECUTE FUNCTION public.set_updated_at();
