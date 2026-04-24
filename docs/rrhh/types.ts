// Tipos del Módulo RRHH — Monarca I+D
// Copiar a MID_Frontend/src/types/rrhh.ts

export type TipoDocumento = 'CC' | 'CE' | 'PA' | 'TI' | 'NIT'

export type Departamento =
  | 'comercial' | 'proyectos' | 'produccion'
  | 'almacen'  | 'rrhh'      | 'gerencia' | 'administrativo'

export type TipoContrato =
  | 'indefinido' | 'fijo' | 'obra_labor'
  | 'prestacion_servicios' | 'aprendiz'

export type RolProyecto =
  | 'lider_proyectos'
  | 'desarrollador'
  | 'coordinador_produccion'
  | 'observador'

export interface Employee {
  id: string
  tenant_id: string
  user_id: string | null          // UUID de la cuenta del sistema, si tiene acceso
  nombres: string
  apellidos: string
  nombre_completo: string         // nombres + apellidos (calculado por el backend)
  tipo_documento: TipoDocumento
  numero_documento: string
  email_personal: string | null
  email_corporativo: string | null
  telefono1: string | null
  telefono2: string | null
  fecha_nacimiento: string | null // "YYYY-MM-DD"
  direccion: string | null
  ciudad: string
  cargo: string
  departamento: Departamento
  tipo_contrato: TipoContrato
  fecha_ingreso: string           // "YYYY-MM-DD"
  fecha_retiro: string | null     // "YYYY-MM-DD"
  salario_base: number | null
  activo: boolean
  notas: string | null
  created_at: string
  updated_at: string
  // Solo en GET /employees/:id
  proyectos_activos?: EmployeeProject[]
}

export interface EmployeeProject {
  project_id: string
  nombre: string
  estado: string
  rol_proyecto: RolProyecto
}

// Miembro de proyecto (viene embebido en GET /projects/:id)
export interface ProjectMember {
  id: string
  rol_proyecto: RolProyecto
  activo: boolean
  assigned_at: string
  employee: {
    id: string
    nombre_completo: string
    cargo: string
    departamento: Departamento
    email_corporativo: string | null
    telefono1: string | null
  }
}

// ── Request Bodies ────────────────────────────────────────────────────────────

export interface CreateEmployeeBody {
  nombres: string
  apellidos: string
  tipo_documento: TipoDocumento
  numero_documento: string
  cargo: string
  departamento: Departamento
  tipo_contrato: TipoContrato
  fecha_ingreso: string           // "YYYY-MM-DD"
  user_id?: string                // Vincular a cuenta existente del sistema
  email_personal?: string
  email_corporativo?: string
  telefono1?: string
  telefono2?: string
  fecha_nacimiento?: string
  direccion?: string
  ciudad?: string
  fecha_retiro?: string
  salario_base?: number
  notas?: string
}

export interface AddProjectMemberBody {
  employee_id: string
  rol_proyecto: RolProyecto
}

// ── Response Wrappers ─────────────────────────────────────────────────────────

export interface EmployeeListResponse { employees: Employee[]; total: number }
export interface MemberListResponse   { members: ProjectMember[]; total: number }
