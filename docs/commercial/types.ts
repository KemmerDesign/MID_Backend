// Tipos del Módulo Comercial — Monarca I+D
// Generado desde los modelos PostgreSQL del backend.
// Usar directamente en src/types/ del frontend.

// ── Clientes ──────────────────────────────────────────────────────────────────

export interface Client {
  id: string
  tenant_id: string
  nombre: string
  nombre_comercial: string | null
  nit: string
  direccion_principal: string
  ciudad_principal: string
  correo_facturacion: string | null
  sitio_web: string | null
  sector: string | null
  activo: boolean
  notas: string | null
  created_at: string
  updated_at: string
  // Solo en GET /clients/:id
  addresses?: ClientAddress[]
  contacts?: ClientContact[]
}

export interface ClientAddress {
  id: string
  client_id: string
  etiqueta: string        // "Sede Norte", "Bodega Central", etc.
  direccion: string
  ciudad: string
  departamento: string | null
  pais: string
  es_principal: boolean
}

export interface ClientContact {
  id: string
  client_id: string
  nombre: string
  cargo: string | null
  email: string | null
  telefono1: string | null
  telefono2: string | null
  es_principal: boolean
}

export interface ClientVisit {
  id: string
  client_id: string
  commercial_id: string
  tenant_id: string
  fecha_visita: string          // "YYYY-MM-DD"
  tipo: 'presencial' | 'virtual' | 'telefonica'
  resumen: string
  compromisos: string | null
  created_at: string
}

// ── Proyectos ─────────────────────────────────────────────────────────────────

export type ProjectEstado =
  | 'borrador'
  | 'en_revision'
  | 'aprobado'
  | 'en_produccion'
  | 'entregado'
  | 'cancelado'

export type Departamento = 'produccion' | 'proyectos' | 'almacen' | 'gerencia'
export type ApprovalEstado = 'pendiente' | 'aprobado' | 'rechazado'

export interface Project {
  id: string
  tenant_id: string
  client_id: string
  commercial_id: string
  nombre: string
  descripcion: string | null
  estado: ProjectEstado
  fecha_prometida: string | null    // "YYYY-MM-DD"
  presupuesto_cliente: number | null
  notas: string | null
  created_at: string
  updated_at: string
  // Solo en GET /projects/:id
  approvals?: ProjectApproval[]
  product_requests?: ProductRequest[]
}

export interface ProjectApproval {
  id: string
  project_id: string
  departamento: Departamento
  approver_id: string | null
  estado: ApprovalEstado
  comentarios: string | null
  fecha_resolucion: string | null
}

// ── Solicitudes de Producto ───────────────────────────────────────────────────

export type ProductRequestEstado = 'pendiente' | 'en_revision' | 'aprobado' | 'rechazado'

export interface ProductRequest {
  id: string
  project_id: string
  tenant_id: string
  commercial_id: string
  nombre_producto: string
  descripcion_necesidad: string
  cantidad_estimada: number
  presupuesto_estimado: number | null
  unidad_medida: string          // "unidad", "metro", "kg", "m²", etc.
  estado: ProductRequestEstado
  notas_adicionales: string | null
  created_at: string
  updated_at: string
}

// ── Request Bodies (para llamadas POST/PATCH) ─────────────────────────────────

export interface CreateClientBody {
  nombre: string
  nombre_comercial?: string
  nit: string
  direccion_principal: string
  ciudad_principal?: string
  correo_facturacion?: string
  sitio_web?: string
  sector?: string
  notas?: string
}

export interface CreateProjectBody {
  client_id: string
  nombre: string
  descripcion?: string
  fecha_prometida?: string      // "YYYY-MM-DD"
  presupuesto_cliente?: number
  notas?: string
}

export interface CreateProductRequestBody {
  nombre_producto: string
  descripcion_necesidad: string
  cantidad_estimada: number
  presupuesto_estimado?: number
  unidad_medida?: string
  notas_adicionales?: string
}

export interface CreateVisitBody {
  client_id: string
  fecha_visita: string          // "YYYY-MM-DD"
  tipo: 'presencial' | 'virtual' | 'telefonica'
  resumen: string
  compromisos?: string
}

export interface ResolveApprovalBody {
  estado: 'aprobado' | 'rechazado'
  comentarios?: string
}

// ── Response Wrappers ─────────────────────────────────────────────────────────

export interface ClientListResponse   { clients: Client[];  total: number }
export interface ProjectListResponse  { projects: Project[]; total: number }
export interface PRListResponse       { product_requests: ProductRequest[]; total: number }
