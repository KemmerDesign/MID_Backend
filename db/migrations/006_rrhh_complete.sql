-- Migración 006: completar módulo RRHH
-- Cubre: inasistencia injustificada (descuento dominical), contactos de emergencia,
-- documentación de vinculación, formación, educación e historial laboral previo.
-- ─────────────────────────────────────────────────────────────────────────────

-- ── 1. Parche employee_absences: inasistencia injustificada + descuento domingo ─
-- Artículo 183 CST (Colombia): una inasistencia injustificada hace perder el
-- descanso remunerado del domingo de esa semana → 2 días de descuento por 1 falta.
-- descuenta_domingo: el responsable de nómina lo activa al registrar/aprobar.
-- domingo_afectado: el domingo concreto que se descuenta (para reportes exactos).

ALTER TABLE public.employee_absences
    DROP CONSTRAINT IF EXISTS employee_absences_tipo_check;

ALTER TABLE public.employee_absences
    ADD CONSTRAINT employee_absences_tipo_check
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
            'inasistencia_injustificada',
            'otro'
        ));

ALTER TABLE public.employee_absences
    ADD COLUMN IF NOT EXISTS descuenta_domingo  BOOLEAN NOT NULL DEFAULT false,
    ADD COLUMN IF NOT EXISTS domingo_afectado   DATE;

-- Garantizar consistencia: si descuenta_domingo=true, domingo_afectado no puede ser nulo
ALTER TABLE public.employee_absences
    ADD CONSTRAINT chk_domingo_fecha
        CHECK (descuenta_domingo = false OR domingo_afectado IS NOT NULL);

-- ── 2. Parche employee_positions: clarificar honorarios vs salario ─────────────
-- Para contratistas la columna salario_base almacena el valor de honorarios.
-- es_honorarios=true le indica a reportes y nómina que no aplican prestaciones sociales.
ALTER TABLE public.employee_positions
    ADD COLUMN IF NOT EXISTS es_honorarios BOOLEAN NOT NULL DEFAULT false;

-- ── 3. Contactos de emergencia ────────────────────────────────────────────────
CREATE TABLE IF NOT EXISTS public.employee_emergency_contacts (
    id          UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    employee_id UUID        NOT NULL REFERENCES public.employees(id) ON DELETE CASCADE,
    tenant_id   UUID        NOT NULL,
    nombres     TEXT        NOT NULL,
    apellidos   TEXT        NOT NULL,
    parentesco  TEXT        NOT NULL,
    telefono1   TEXT        NOT NULL,
    telefono2   TEXT,
    email       TEXT,
    es_principal BOOLEAN    NOT NULL DEFAULT false,
    created_at  TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at  TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_emerg_contacts_employee ON public.employee_emergency_contacts (employee_id);

-- Solo puede haber un contacto principal por empleado
CREATE UNIQUE INDEX IF NOT EXISTS uq_emerg_contact_principal
    ON public.employee_emergency_contacts (employee_id)
    WHERE es_principal = true;

-- ── 4. Documentos de vinculación ──────────────────────────────────────────────
-- Permite rastrear qué documentos obligatorios ha entregado el empleado y cuáles
-- están pendientes, para auditoría de RRHH y cumplimiento legal.
CREATE TABLE IF NOT EXISTS public.employee_documents (
    id              UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    employee_id     UUID        NOT NULL REFERENCES public.employees(id) ON DELETE CASCADE,
    tenant_id       UUID        NOT NULL,
    tipo            TEXT        NOT NULL
        CHECK (tipo IN (
            'cedula_ciudadania',
            'hoja_de_vida',
            'diploma_bachiller',
            'diploma_tecnico',
            'diploma_universitario',
            'certificado_laboral',
            'certificado_eps',
            'certificado_afp',
            'examen_medico_ingreso',
            'antecedentes_penales',
            'antecedentes_disciplinarios',
            'certificacion_altura',
            'certificacion_soldadura',
            'contrato_firmado',
            'acta_inicio',
            'otro'
        )),
    nombre          TEXT        NOT NULL,
    url_referencia  TEXT,
    fecha_entrega   DATE,
    fecha_vencimiento DATE,
    obligatorio     BOOLEAN     NOT NULL DEFAULT true,
    estado          TEXT        NOT NULL DEFAULT 'pendiente'
        CHECK (estado IN ('pendiente','recibido','vencido','no_aplica')),
    notas           TEXT,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_emp_documents_employee ON public.employee_documents (employee_id);
CREATE INDEX IF NOT EXISTS idx_emp_documents_estado   ON public.employee_documents (estado);

-- ── 5. Formación y capacitaciones ─────────────────────────────────────────────
-- En manufactura/metalurgia las certificaciones tienen vencimiento (alturas, soldadura).
-- fecha_vencimiento NULL = no vence (curso académico, inducción, etc.).
CREATE TABLE IF NOT EXISTS public.employee_training (
    id                UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    employee_id       UUID        NOT NULL REFERENCES public.employees(id) ON DELETE CASCADE,
    tenant_id         UUID        NOT NULL,
    nombre_curso      TEXT        NOT NULL,
    institucion       TEXT,
    tipo              TEXT        NOT NULL
        CHECK (tipo IN (
            'induccion','capacitacion_interna','capacitacion_externa',
            'certificacion','curso_virtual','otro'
        )),
    modalidad         TEXT
        CHECK (modalidad IN ('presencial','virtual','mixta')),
    fecha_inicio      DATE        NOT NULL,
    fecha_fin         DATE,
    horas             INTEGER,
    resultado         TEXT        CHECK (resultado IN ('aprobado','reprobado','en_curso','pendiente')),
    fecha_vencimiento DATE,
    certificado_url   TEXT,
    notas             TEXT,
    created_at        TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at        TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_emp_training_employee   ON public.employee_training (employee_id);
CREATE INDEX IF NOT EXISTS idx_emp_training_vencimiento ON public.employee_training (fecha_vencimiento)
    WHERE fecha_vencimiento IS NOT NULL;

-- ── 6. Formación académica (hoja de vida) ─────────────────────────────────────
CREATE TABLE IF NOT EXISTS public.employee_education (
    id              UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    employee_id     UUID        NOT NULL REFERENCES public.employees(id) ON DELETE CASCADE,
    tenant_id       UUID        NOT NULL,
    nivel           TEXT        NOT NULL
        CHECK (nivel IN (
            'bachillerato','tecnico','tecnologo','universitario',
            'especializacion','maestria','doctorado','otro'
        )),
    titulo          TEXT        NOT NULL,
    institucion     TEXT        NOT NULL,
    anio_inicio     INTEGER,
    anio_graduacion INTEGER,
    en_curso        BOOLEAN     NOT NULL DEFAULT false,
    ciudad          TEXT,
    notas           TEXT,
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT now()
);

CREATE INDEX IF NOT EXISTS idx_emp_education_employee ON public.employee_education (employee_id);

-- ── 7. Experiencia laboral previa (hoja de vida) ──────────────────────────────
-- fecha_fin NULL = trabajo actual (no debería coincidir con la empresa del ERP,
-- pero es posible si el empleado tiene otro trabajo simultáneo permitido).
CREATE TABLE IF NOT EXISTS public.employee_work_history (
    id              UUID        PRIMARY KEY DEFAULT gen_random_uuid(),
    employee_id     UUID        NOT NULL REFERENCES public.employees(id) ON DELETE CASCADE,
    tenant_id       UUID        NOT NULL,
    empresa         TEXT        NOT NULL,
    cargo           TEXT        NOT NULL,
    departamento    TEXT,
    fecha_inicio    DATE        NOT NULL,
    fecha_fin       DATE,
    descripcion     TEXT,
    motivo_retiro   TEXT
        CHECK (motivo_retiro IN (
            'renuncia_voluntaria','terminacion_contrato','mutuo_acuerdo',
            'despido_justa_causa','despido_sin_justa_causa','traslado',
            'fin_proyecto','otro'
        )),
    created_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    updated_at      TIMESTAMPTZ NOT NULL DEFAULT now(),
    CONSTRAINT chk_work_dates CHECK (fecha_fin IS NULL OR fecha_fin > fecha_inicio)
);

CREATE INDEX IF NOT EXISTS idx_emp_work_history_employee ON public.employee_work_history (employee_id);

-- ── Triggers updated_at ───────────────────────────────────────────────────────
CREATE TRIGGER trg_emerg_contacts_updated_at
    BEFORE UPDATE ON public.employee_emergency_contacts
    FOR EACH ROW EXECUTE FUNCTION public.set_updated_at();

CREATE TRIGGER trg_emp_documents_updated_at
    BEFORE UPDATE ON public.employee_documents
    FOR EACH ROW EXECUTE FUNCTION public.set_updated_at();

CREATE TRIGGER trg_emp_training_updated_at
    BEFORE UPDATE ON public.employee_training
    FOR EACH ROW EXECUTE FUNCTION public.set_updated_at();

CREATE TRIGGER trg_emp_education_updated_at
    BEFORE UPDATE ON public.employee_education
    FOR EACH ROW EXECUTE FUNCTION public.set_updated_at();

CREATE TRIGGER trg_emp_work_history_updated_at
    BEFORE UPDATE ON public.employee_work_history
    FOR EACH ROW EXECUTE FUNCTION public.set_updated_at();
