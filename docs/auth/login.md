# Endpoint: Login de Usuario

## Resumen

Autentica a un usuario registrado y devuelve un JWT de sesión válido por 24 horas.

---

## Contrato HTTP

| Campo       | Valor                      |
|-------------|----------------------------|
| Método      | `POST`                     |
| Ruta        | `/api/v1/auth/login`       |
| Content-Type| `application/json`         |
| Auth requerida | No                      |

---

## Request Body

```json
{
  "email":    "usuario@empresa.com",
  "password": "contraseña_plana"
}
```

| Campo      | Tipo   | Requerido | Descripción                        |
|------------|--------|-----------|------------------------------------|
| `email`    | string | Sí        | Email registrado del usuario        |
| `password` | string | Sí        | Contraseña en texto plano (se hashea en servidor) |

---

## Respuestas

### 200 OK — Autenticación exitosa

```json
{
  "token": "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9...",
  "user": {
    "id":        "550e8400-e29b-41d4-a716-446655440000",
    "email":     "usuario@empresa.com",
    "full_name": "Juan Pérez",
    "role":      "admin",
    "tenant_id": "7d3f1200-c4ab-4e8d-9b3c-112233445566"
  }
}
```

### 401 Unauthorized — Credenciales inválidas

```json
{ "error": "Credenciales inválidas" }
```

> Se devuelve el mismo mensaje tanto si el email no existe como si la contraseña es incorrecta. Esto evita enumerar usuarios registrados.

### 400 Bad Request — Campos faltantes o mal formados

```json
{ "error": "Campo requerido: email" }
```

### 403 Forbidden — Usuario desactivado

```json
{ "error": "Cuenta desactivada. Contacte al administrador." }
```

### 500 Internal Server Error — Error de servidor

```json
{ "error": "Error interno del servidor" }
```

---

## JWT — Claims del token

| Claim       | Tipo   | Descripción                          |
|-------------|--------|--------------------------------------|
| `iss`       | string | `"monarca"`                          |
| `user_id`   | string | UUID del usuario                     |
| `tenant_id` | string | UUID del tenant (empresa cliente)    |
| `role`      | string | Rol del usuario (`admin`, `employee`, etc.) |
| `exp`       | number | Unix timestamp de expiración (24h)   |
| `iat`       | number | Unix timestamp de emisión            |

El JWT se firma con HS256. La clave secreta se lee de la variable de entorno `MONARCA_JWT_SECRET` (fallback: `monarca_dev_secret_CHANGE_IN_PROD`).

---

## Tabla PostgreSQL requerida

```sql
public.users
```

Ver migración: `db/migrations/001_create_users.sql`

---

## Seguridad

- Las contraseñas se almacenan como `PBKDF2-SHA256` con salt aleatorio de 16 bytes y 100.000 iteraciones.
- El formato almacenado es `<salt_hex>:<hash_hex>`.
- Nunca se devuelve `password_hash` en ninguna respuesta.
- El token **no** se almacena en servidor (stateless). Para revocar sesiones, se implementará una lista negra de JTIs en Redis (fase futura).

---

## Uso desde Frontend

```typescript
// Ejemplo con fetch
const response = await fetch('http://localhost:8080/api/v1/auth/login', {
  method: 'POST',
  headers: { 'Content-Type': 'application/json' },
  body: JSON.stringify({ email, password })
});

const data = await response.json();
if (response.ok) {
  localStorage.setItem('monarca_token', data.token);
  // Adjuntar en todas las peticiones protegidas:
  // Authorization: Bearer <token>
}
```

---

## Endpoints protegidos (próximos)

Los endpoints que requieren autenticación validarán el header `Authorization: Bearer <token>` mediante un filtro Drogon que decodifica el JWT y adjunta los claims al `HttpRequest`.
