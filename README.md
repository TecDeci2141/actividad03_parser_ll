# Actividad 3 — Analizador descendente LL(1) en C

**Materia:** Programación de Sistemas
**Unidad:** 4. Análisis sintáctico
**Unidad de Estudios Superiores:** Jiquipilco
**Universidad:** Universitaria Mexiquense del Bicentenario
**Ingeniería:** Ingeniería en Sistemas Computacionales

| Dato | Valor |
|---|---|
| Matrícula | `10240041` |
| Nombre completo | DIEGO DECIDERIO GARCÍA |
| Grupo | 10SC1051 |
| Fecha de entrega | martes 27 de octubre de 2026 |
| Repositorio | [https://github.com/TecDeci2141/actividad03_parser_ll.git] |

---

## 1. Objetivo

Construir, en C11, un reconocedor sintáctico descendente LL(1) que, a partir de una
Gramática Libre de Contexto (GLC) de expresiones aritméticas, acepte o rechace
secuencias de tokens preclasificados, respetando la precedencia de operadores y
deteniéndose en el primer desacuerdo sintáctico. El programa **no** realiza análisis
léxico, no construye AST, no evalúa expresiones ni verifica tipos: su única
responsabilidad es validar el orden estructural de los tokens.

---

## 2. Gramáticas

### 2.1 Gramática base

```
E  → T E'
E' → + T E' | - T E' | ε
T  → F T'
T' → * F T' | / F T' | ε
F  → ID | NUM | ( E )
```

- **Símbolo inicial:** `E`
- **Terminales:** `ID, NUM, +, -, *, /, (, ), $`
- **No terminales:** `E, E', T, T', F`

### 2.2 Gramática modificada (con menos unario)

Se amplía `F` para reconocer el operador unario `-`:

```
F → - F | ID | NUM | ( E )
```

El resto de las producciones permanece idéntico.

---

## 3. Vocabulario

| Concepto | Definición en esta práctica |
|---|---|
| **Terminal** | Símbolo que aparece directamente en la entrada. Ejemplos: `ID`, `NUM`, `+`, `-`, `*`, `/`, `(`, `)`, `$`. |
| **No terminal** | Categoría abstracta definida por la gramática. Ejemplos: `E`, `E'`, `T`, `T'`, `F`. |
| **Token** | Categoría del símbolo ya clasificado. Ejemplo: `TOK_ID`. |
| **Lexema** | Texto asociado al token. Ejemplo: `"id"`, `"num"`, `"+"`. |
| **Producción** | Regla que indica cómo puede desarrollarse un no terminal. Ejemplo: `E → T E'`. |
| **ε (épsilon)** | Producción vacía: no consume ningún token ni avanza el cursor. |
| **Lookahead** | Token actual que permite decidir qué producción aplicar. LL significa *Left-to-right, Leftmost derivation*. |

---

## 4. Construcción y ejecución

### 4.1 Compilación estricta

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -Werror parser_ll.c -o parser_ll
```

### 4.2 Suite base (8/8)

```bash
./parser_ll | tee evidencias/salida_base.txt
```

Resultado obtenido:

```
RESUMEN pruebas=8 aciertos=8 fallos=0
```

### 4.3 Suite modificada con menos unario (11/11)

```bash
./parser_ll | tee evidencias/salida_modificada.txt
```

Resultado obtenido:

```
RESUMEN pruebas=11 aciertos=11 fallos=0
```

### 4.4 Sanitizadores

```bash
gcc -std=c11 -Wall -Wextra -Wpedantic -O0 -g \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    parser_ll.c -o parser_ll_san
./parser_ll_san > evidencias/sanitizadores.txt 2>&1
```

Resultado: `estado_sanitizadores=0` y sin diagnósticos de `AddressSanitizer` ni
`runtime error`.

---

## 5. Preguntas de razonamiento

### Pregunta 1 — Recursión izquierda

En un descenso recursivo, la función `expresion()` representaría a `E`. Si la
producción fuera `E → E + T | T`, la función se llamaría a sí misma como primera
acción antes de consumir ningún token, generando un ciclo infinito sin avance del
cursor. La forma equivalente `E → T E'` obliga a consumir primero un `T`, lo que
rompe el ciclo y permite el descenso controlado.

### Pregunta 2 — Precedencia en el caso 1 (`id + num * id`)

Las líneas de traza muestran que, tras consumir `+`, se invoca `termino()` y dentro
de él se procesa completo `num * id` antes de regresar a `resto_expresion()`:

```
[pos=2, lookahead=NUM "num"] T -> F T'
[pos=3, lookahead=POR "*"]   T' -> * F T'
[pos=5, lookahead=FIN "$"]   T' -> epsilon
[pos=5, lookahead=FIN "$"]   E' -> epsilon
```

La producción `T → F T'` agrupa `num * id` dentro de `T`, por lo que `*` tiene
mayor precedencia que `+`.

### Pregunta 3 — Agrupación en el caso 2 (`(id + num) * id`)

La producción `F → ( E )` obliga a resolver la expresión interna antes de regresar
al término externo. En la traza se observa que el `+` se consume dentro de la
llamada recursiva a `expresion()` y solo después se regresa a `T'` para aplicar
`T' → * F T'`:

```
[pos=1, lookahead=ID "id"]   F -> ( E )
[pos=3, lookahead=MAS "+"]   E' -> + T E'
...
[pos=5, lookahead=POR "*"]   T' -> * F T'
```

Esto confirma que la suma entre paréntesis se procesa antes que la multiplicación
externa.

### Pregunta 4 — Producción ε

La producción ε no consume token porque su cuerpo es vacío: en el código, las
funciones `resto_termino()` y `resto_expresion()` simplemente devuelven `1` sin
invocar `consumir()`, por lo que `p->posicion` permanece intacta. Se elige con los
lookaheads que no inician otra alternativa: para `T'` con `+, -, ), $`; para `E'`
con `), $`.

### Pregunta 5 — Error en el caso 5 (`id + * num`)

El error se detecta en `factor()` cuando el lookahead es `TOK_POR` en `pos=2`. El
diagnóstico emitido es:

```
F no tiene produccion para POR "*" en pos=2.
```

Ninguna rama de `F` acepta `*`, por lo que la función devuelve `0` y la cadena se
rechaza sin retroceso.

### Pregunta 6 — Casos 7 y 8

- **Caso 7 (`id ) $`):** `factor()` consume `id`, `resto_termino()` y
  `resto_expresion()` aplican ε, y entonces `analizar()` invoca
  `consumir(&p, TOK_FIN)`. Como el token actual es `)`, se emite
  `Se esperaba FIN y se encontro PAR_DER ")" en pos=1.`
- **Caso 8 (`id $ id`):** tras consumir `id` y `FIN`, la comprobación
  `p.posicion != p.cantidad` dentro de `analizar()` detecta que aún queda un token
  y emite `Existen tokens despues de FIN.` Esto impide aceptar un prefijo válido.

### Pregunta 7 — Menos unario

Con `F → - F | ID | NUM | ( E )`, la entrada `- id * num $` es válida porque `F`
consume `-`, llama recursivamente a `F` para aceptar `id`, y luego `T'` aplica
`T' → * F T'` para consumir `* num`. En cambio, `- $` se rechaza porque tras
consumir `-`, `factor()` se invoca nuevamente y su lookahead es `TOK_FIN`, que no
coincide con `ID`, `NUM`, `(` ni `-`; el diagnóstico es
`Se esperaba ID o NUM; no quedan tokens.`

### Pregunta 8 — Frontera del análisis sintáctico

Aceptar una expresión solo garantiza que los tokens aparecen en un orden permitido
por la GLC. No verifica que los identificadores estén declarados en la tabla de
símbolos, ni que los operandos de `+` o `*` sean de tipos compatibles, ni que una
variable se use antes de asignársele valor. Esas comprobaciones pertenecen a las
fases semántica y de análisis de tipos, que ocurren después del análisis sintáctico.

---

## 6. Conclusión

El parser construido demuestra que una GLC puede traducirse directamente a un
conjunto de funciones mutuamente recursivas donde cada no terminal es una función y
el lookahead decide la producción. Se comprobó la precedencia de `*` y `/` sobre
`+` y `-`, el agrupamiento con paréntesis, la extensión con menos unario y el
rechazo de errores sintácticos comunes. Quedan fuera del alcance de esta práctica
el análisis léxico, la construcción del AST, la evaluación de expresiones, la
administración de la tabla de símbolos, la verificación de tipos, la recuperación
de errores y la generación de código intermedio o objeto; esas responsabilidades
corresponden a fases posteriores del compilador.

---

## 7. Estructura del repositorio

```
actividad03_parser_ll/
├── parser_ll.c
├── README.md
├── .gitignore
├── Matricula10272026.pdf
└── evidencias/
    ├── salida_base.txt
    ├── salida_modificada.txt
    └── sanitizadores.txt
```

---

## 8. Historial de commits

```
git log --oneline --decorate -3
```

```
<hash> (HEAD -> main, origin/main) docs: completar README y URL del repositorio
<hash> test: agregar menos unario y regresion
<hash> feat: implementar descenso recursivo y pruebas base
<hash> feat: representar gramatica y cursor LL
```

---

## 9. URL del repositorio

[https://github.com/TecDeci2141/actividad03_parser_ll.git]