/*
 * Matricula: 10240041
 * Nombre completo: DIEGO DECIDERIO GARCIA
 * Actividad 3: gramatica de expresiones y analizador descendente LL
 */

#include <stdio.h>
#include <stddef.h>

typedef enum {
    TOK_ID, TOK_NUM, TOK_MAS, TOK_MENOS, TOK_POR, TOK_DIV,
    TOK_PAR_IZQ, TOK_PAR_DER, TOK_FIN, TOK_OTRO
} TipoToken;

typedef struct {
    TipoToken tipo;
    const char *lexema;
} Token;

typedef struct {
    const Token *tokens;
    size_t cantidad;
    size_t posicion;
    int mostrar_traza;
} Parser;

typedef struct {
    const char *nombre;
    const Token *tokens;
    size_t cantidad;
    int esperado;
    int mostrar_traza;
} CasoPrueba;

const char *nombre_token(TipoToken tipo)
{
    switch (tipo) {
        case TOK_ID: return "ID";
        case TOK_NUM: return "NUM";
        case TOK_MAS: return "MAS";
        case TOK_MENOS: return "MENOS";
        case TOK_POR: return "POR";
        case TOK_DIV: return "DIV";
        case TOK_PAR_IZQ: return "PAR_IZQ";
        case TOK_PAR_DER: return "PAR_DER";
        case TOK_FIN: return "FIN";
        case TOK_OTRO: return "OTRO";
    }
    return "DESCONOCIDO";
}

const Token *token_actual(const Parser *p)
{
    if (p->posicion >= p->cantidad) {
        return NULL;
    }
    return &p->tokens[p->posicion];
}

void trazar(const Parser *p, const char *regla)
{
    const Token *t;
    if (!p->mostrar_traza) {
        return;
    }
    t = token_actual(p);
    if (t == NULL) {
        printf("  [pos=%zu, sin token] %s\n", p->posicion, regla);
        return;
    }
    printf("  [pos=%zu, lookahead=%s \"%s\"] %s\n",
           p->posicion, nombre_token(t->tipo), t->lexema, regla);
}

int consumir(Parser *p, TipoToken esperado)
{
    const Token *t = token_actual(p);
    if (t == NULL) {
        printf("  [ERROR sintactico] Se esperaba %s, pero no quedan tokens.\n",
               nombre_token(esperado));
        return 0;
    }
    if (t->tipo != esperado) {
        printf("  [ERROR sintactico] Se esperaba %s y se encontro %s \"%s\" en pos=%zu.\n",
               nombre_token(esperado), nombre_token(t->tipo),
               t->lexema, p->posicion);
        return 0;
    }
    if (p->mostrar_traza) {
        printf("  [consume] %s \"%s\"\n", nombre_token(t->tipo), t->lexema);
    }
    p->posicion++;
    return 1;
}

int expresion(Parser *p);

int factor(Parser *p)
{
    const Token *t = token_actual(p);
    if (t == NULL) {
        printf("  [ERROR sintactico] Se esperaba ID, NUM, PAR_IZQ o MENOS; no quedan tokens.\n");
        return 0;
    }
    if (t->tipo == TOK_MENOS) {
        trazar(p, "F -> - F");
        if (!consumir(p, TOK_MENOS)) {
            return 0;
        }
        return factor(p);
    }
    if (t->tipo == TOK_ID) {
        trazar(p, "F -> ID");
        return consumir(p, TOK_ID);
    }
    if (t->tipo == TOK_NUM) {
        trazar(p, "F -> NUM");
        return consumir(p, TOK_NUM);
    }
    if (t->tipo == TOK_PAR_IZQ) {
        trazar(p, "F -> ( E )");
        if (!consumir(p, TOK_PAR_IZQ)) {
            return 0;
        }
        if (!expresion(p)) {
            return 0;
        }
        return consumir(p, TOK_PAR_DER);
    }
    printf("  [ERROR sintactico] F no tiene produccion para %s \"%s\" en pos=%zu.\n",
           nombre_token(t->tipo), t->lexema, p->posicion);
    return 0;
}

int resto_termino(Parser *p)
{
    const Token *t = token_actual(p);
    if (t != NULL && t->tipo == TOK_POR) {
        trazar(p, "T' -> * F T'");
        if (!consumir(p, TOK_POR) || !factor(p)) {
            return 0;
        }
        return resto_termino(p);
    }
    if (t != NULL && t->tipo == TOK_DIV) {
        trazar(p, "T' -> / F T'");
        if (!consumir(p, TOK_DIV) || !factor(p)) {
            return 0;
        }
        return resto_termino(p);
    }
    if (t != NULL &&
        (t->tipo == TOK_MAS || t->tipo == TOK_MENOS ||
         t->tipo == TOK_PAR_DER || t->tipo == TOK_FIN)) {
        trazar(p, "T' -> epsilon");
        return 1;
    }
    if (t == NULL) {
        printf("  [ERROR sintactico] T' requiere un token de continuacion.\n");
        return 0;
    }
    printf("  [ERROR sintactico] T' no tiene produccion para %s \"%s\" en pos=%zu.\n",
           nombre_token(t->tipo), t->lexema, p->posicion);
    return 0;
}

int termino(Parser *p)
{
    trazar(p, "T -> F T'");
    return factor(p) && resto_termino(p);
}

int resto_expresion(Parser *p)
{
    const Token *t = token_actual(p);
    if (t != NULL && t->tipo == TOK_MAS) {
        trazar(p, "E' -> + T E'");
        if (!consumir(p, TOK_MAS) || !termino(p)) {
            return 0;
        }
        return resto_expresion(p);
    }
    if (t != NULL && t->tipo == TOK_MENOS) {
        trazar(p, "E' -> - T E'");
        if (!consumir(p, TOK_MENOS) || !termino(p)) {
            return 0;
        }
        return resto_expresion(p);
    }
    if (t != NULL && (t->tipo == TOK_PAR_DER || t->tipo == TOK_FIN)) {
        trazar(p, "E' -> epsilon");
        return 1;
    }
    if (t == NULL) {
        printf("  [ERROR sintactico] E' requiere un token de continuacion.\n");
        return 0;
    }
    printf("  [ERROR sintactico] E' no tiene produccion para %s \"%s\" en pos=%zu.\n",
           nombre_token(t->tipo), t->lexema, p->posicion);
    return 0;
}

int expresion(Parser *p)
{
    trazar(p, "E -> T E'");
    return termino(p) && resto_expresion(p);
}

int analizar(const Token tokens[], size_t cantidad, int mostrar_traza)
{
    Parser p = {tokens, cantidad, 0U, mostrar_traza};
    if (!expresion(&p)) {
        return 0;
    }
    if (!consumir(&p, TOK_FIN)) {
        return 0;
    }
    if (p.posicion != p.cantidad) {
        printf("  [ERROR sintactico] Existen tokens despues de FIN.\n");
        return 0;
    }
    return 1;
}

int ejecutar_caso(const CasoPrueba *caso)
{
    int obtenido;
    int coincide;
    printf("\n=== %s ===\n", caso->nombre);
    obtenido = analizar(caso->tokens, caso->cantidad, caso->mostrar_traza);
    coincide = (obtenido == caso->esperado);
    printf("Resultado: %s | Esperado: %s | %s\n",
           obtenido ? "ACEPTADA" : "RECHAZADA",
           caso->esperado ? "ACEPTADA" : "RECHAZADA",
           coincide ? "OK" : "FALLO");
    return coincide;
}

#define CANTIDAD(a) (sizeof(a) / sizeof((a)[0]))

int main(void)
{
    const Token caso1[] = {
        {TOK_ID, "id"}, {TOK_MAS, "+"}, {TOK_NUM, "num"},
        {TOK_POR, "*"}, {TOK_ID, "id"}, {TOK_FIN, "$"}
    };
    const Token caso2[] = {
        {TOK_PAR_IZQ, "("}, {TOK_ID, "id"}, {TOK_MAS, "+"},
        {TOK_NUM, "num"}, {TOK_PAR_DER, ")"}, {TOK_POR, "*"},
        {TOK_ID, "id"}, {TOK_FIN, "$"}
    };
    const Token caso3[] = {
        {TOK_ID, "id"}, {TOK_MENOS, "-"}, {TOK_NUM, "num"},
        {TOK_DIV, "/"}, {TOK_ID, "id"}, {TOK_FIN, "$"}
    };
    const Token caso4[] = {
        {TOK_ID, "id"}, {TOK_FIN, "$"}
    };
    const Token caso5[] = {
        {TOK_ID, "id"}, {TOK_MAS, "+"}, {TOK_POR, "*"},
        {TOK_NUM, "num"}, {TOK_FIN, "$"}
    };
    const Token caso6[] = {
        {TOK_PAR_IZQ, "("}, {TOK_ID, "id"}, {TOK_MAS, "+"},
        {TOK_NUM, "num"}, {TOK_FIN, "$"}
    };
    const Token caso7[] = {
        {TOK_ID, "id"}, {TOK_PAR_DER, ")"}, {TOK_FIN, "$"}
    };
    const Token caso8[] = {
        {TOK_ID, "id"}, {TOK_FIN, "$"}, {TOK_ID, "id_despues"}
    };
    const Token caso9[] = {
        {TOK_MENOS, "-"}, {TOK_ID, "id"}, {TOK_POR, "*"},
        {TOK_NUM, "num"}, {TOK_FIN, "$"}
    };
    const Token caso10[] = {
        {TOK_ID, "id"}, {TOK_MAS, "+"}, {TOK_MENOS, "-"},
        {TOK_NUM, "num"}, {TOK_FIN, "$"}
    };
    const Token caso11[] = {
        {TOK_MENOS, "-"}, {TOK_FIN, "$"}
    };

    const CasoPrueba casos[] = {
        {"1. precedencia: id + num * id", caso1, CANTIDAD(caso1), 1, 1},
        {"2. parentesis: (id + num) * id", caso2, CANTIDAD(caso2), 1, 1},
        {"3. resta y division: id - num / id", caso3, CANTIDAD(caso3), 1, 0},
        {"4. expresion minima: id", caso4, CANTIDAD(caso4), 1, 0},
        {"5. operadores consecutivos: id + * num", caso5, CANTIDAD(caso5), 0, 1},
        {"6. falta parentesis derecho", caso6, CANTIDAD(caso6), 0, 0},
        {"7. cierre inesperado: id )", caso7, CANTIDAD(caso7), 0, 0},
        {"8. tokens despues de FIN: id $ id", caso8, CANTIDAD(caso8), 0, 0},
        {"9. menos unario: -id * num", caso9, CANTIDAD(caso9), 1, 1},
        {"10. menos unario despues de suma", caso10, CANTIDAD(caso10), 1, 0},
        {"11. menos unario sin operando", caso11, CANTIDAD(caso11), 0, 0}
    };

    size_t i;
    int aciertos = 0;
    for (i = 0U; i < CANTIDAD(casos); i++) {
        aciertos += ejecutar_caso(&casos[i]);
    }
    printf("\nRESUMEN pruebas=%zu aciertos=%d fallos=%zu\n",
           CANTIDAD(casos), aciertos,
           CANTIDAD(casos) - (size_t)aciertos);
    return aciertos == (int)CANTIDAD(casos) ? 0 : 1;
}