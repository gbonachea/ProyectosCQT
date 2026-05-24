#!/usr/bin/env python3
"""
Gemma3 LLM Engine for Busqueda AI
===================================
Optional AI backend that uses Gemma3 1B IT model via LiteRT
to understand natural language search queries.

This script is called by the C++ application via QProcess
to extract structured search criteria from user queries.

Output format (JSON):
{
    "keywords": ["word1", "word2"],
    "file_category": "image|document|video|audio|all",
    "exact_phrases": ["phrase1"],
    "search_content": false,
    "success": true,
    "error": null
}
"""

import json
import sys
import os
import re
import argparse
from pathlib import Path


# ---------------------------------------------------------------------------
#  Fallback parser (used when LLM is not available)
# ---------------------------------------------------------------------------

SPANISH_STOP_WORDS = {
    'un', 'una', 'unos', 'unas', 'el', 'la', 'los', 'las', 'lo',
    'de', 'del', 'en', 'por', 'para', 'con', 'sin', 'sobre', 'entre',
    'y', 'e', 'o', 'u', 'pero', 'sino', 'que', 'como', 'cual',
    'quien', 'donde', 'cuando', 'cuanto', 'se', 'su', 'sus',
    'le', 'les', 'me', 'te', 'nos', 'os', 'mi', 'tu',
    'a', 'ante', 'bajo', 'cabe', 'contra', 'desde', 'durante',
    'hacia', 'hasta', 'mediante', 'segun', 'tras', 'via',
    'es', 'son', 'era', 'fue', 'sera', 'seran', 'esta', 'estan',
    'este', 'esta', 'estos', 'estas', 'ese', 'esa', 'esos', 'esas',
    'aquel', 'aquella', 'aquellos', 'aquellas', 'muy', 'mas', 'menos',
    'tambien', 'tan', 'tanto', 'algo', 'nada', 'todo', 'siempre',
    'nunca', 'tiene', 'tienen', 'puede', 'pueden', 'hay', 'hace',
    'busca', 'buscar', 'encuentra', 'encontrar', 'necesito',
    'quiero', 'podrias', 'puedes', 'ayuda', 'ayudar', 'archivo',
    'archivos', 'fichero', 'ficheros', 'carpeta', 'carpetas',
    'directorio', 'directorios', 'ubicado', 'llamado', 'nombre',
    'tipo', 'tipos', 'donde', 'cual', 'algun', 'alguna',
    'ver', 'mostrar', 'abrir', 'tengo', 'tener',
    'please', 'find', 'search', 'look', 'for', 'the', 'a', 'an',
    'in', 'at', 'with', 'and', 'or', 'of', 'to', 'is', 'are',
    'was', 'were', 'will', 'can', 'could', 'would', 'should',
    'have', 'has', 'had', 'do', 'does', 'did', 'this', 'that',
    'these', 'those', 'i', 'my', 'me', 'you', 'your', 'it', 'its',
    'we', 'our', 'they', 'them', 'their', 'file', 'folder',
    'directory', 'called', 'named', 'show', 'open', 'need', 'want',
}

CATEGORY_KEYWORDS = {
    'image': [
        'foto', 'fotografia', 'fotografía', 'imagen', 'imagenes', 'imágenes',
        'captura', 'pantalla', 'screenshot', 'fotos', 'photo', 'photos',
        'picture', 'pictures', 'image', 'images', 'grafico', 'gráfico',
        'dibujo', 'ilustración', 'ilustracion',
    ],
    'document': [
        'documento', 'documentos', 'document', 'documents', 'pdf',
        'word', 'texto', 'txt', 'archivo', 'hoja', 'calculo',
        'cálculo', 'excel', 'presentacion', 'presentación',
        'powerpoint', 'diapositiva', 'slide', 'spreadsheet',
        'hoja de calculo', 'hoja de cálculo', 'lectura',
        'libro', 'novela', 'articulo', 'artículo', 'escrito',
        'carta', 'formulario', 'informe', 'reporte',
    ],
    'video': [
        'video', 'videos', 'vídeo', 'vídeos', 'pelicula', 'película',
        'movie', 'movies', 'film', 'films', 'clip', 'grabacion',
        'grabación', 'corto', 'documental', 'serie',
        'episodio', 'animacion', 'animación',
    ],
    'audio': [
        'audio', 'audios', 'musica', 'música', 'music', 'song',
        'cancion', 'canción', 'canciones', 'podcast', 'grabacion',
        'grabación', 'sonido', 'sound', 'voz', 'voice', 'album',
        'álbum', 'playlist', 'lista',
    ],
}


def fallback_parse(query: str) -> dict:
    """Simple NLP fallback when LLM is not available."""
    q = query.lower().strip()
    if not q:
        return {"error": "Empty query"}

    search_content = any(word in q for word in [
        'dice', 'contiene', 'contenga', 'menciona', 'habla',
        'texto', 'digan', 'diga', 'contengan',
    ])

    # Detect file category
    file_category = 'all'
    for cat, words in CATEGORY_KEYWORDS.items():
        for word in words:
            if word in q:
                file_category = cat
                break
        if file_category != 'all':
            break

    # Extract exact phrases (quoted text)
    exact_phrases = re.findall(r'["""]([^"""]+)["""]', q)
    exact_phrases = [p.strip().lower() for p in exact_phrases if p.strip()]

    # Extract keywords: remove stop words and category words
    words = re.findall(r'[a-záéíóúüñ0-9]+', q)
    keywords = []
    for w in words:
        if len(w) < 2:
            continue
        if w in SPANISH_STOP_WORDS:
            continue
        # Skip category words
        is_cat = False
        for cat_words in CATEGORY_KEYWORDS.values():
            if w in cat_words:
                is_cat = True
                break
        if is_cat:
            continue
        if w not in keywords:
            keywords.append(w)

    return {
        "keywords": keywords,
        "file_category": file_category,
        "exact_phrases": exact_phrases,
        "search_content": search_content,
        "success": True,
        "error": None,
    }


# ---------------------------------------------------------------------------
#  LLM-based parser (tries to use Gemma3 via LiteRT)
# ---------------------------------------------------------------------------

def llm_parse(query: str, model_path: str) -> dict:
    """Use Gemma3 1B model to parse the query."""
    try:
        import ai_edge_litert as litert
        from ai_edge_litert.compiled_model import (
            CompiledModel, HardwareAccelerator, Options, CpuOptions,
        )
    except ImportError:
        return {"success": False, "error": "ai_edge_litert not installed",
                "fallback": True}

    if not os.path.exists(model_path):
        return {"success": False, "error": f"Model not found: {model_path}",
                "fallback": True}

    try:
        opts = Options(
            hardware_accelerators=HardwareAccelerator.CPU,
            cpu_options=CpuOptions(num_threads=2),
        )
        model = CompiledModel.from_file(model_path, options=opts)

        # Build a prompt for the model to extract search intent
        prompt = (
            "<start_of_turn>user\n"
            "Extract search keywords and file type from this query. "
            "Respond ONLY with a JSON object containing: "
            "'keywords' (array of important words), "
            "'file_category' (one of: image, document, video, audio, all), "
            "'search_content' (true/false - true if user wants to search inside files).\n\n"
            f"Query: {query}\n\n"
            "JSON response:\n"
            "<end_of_turn>\n"
            "<start_of_turn>model\n"
        )

        # TODO: Tokenize prompt and run inference
        # The model requires specific tokenization.
        # For now, fall back to the NLP parser.
        return {"success": False, "error": "LLM inference not yet implemented",
                "fallback": True}

    except Exception as e:
        return {"success": False, "error": str(e), "fallback": True}


# ---------------------------------------------------------------------------
#  Main entry point
# ---------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(description="Busqueda AI LLM Engine")
    parser.add_argument("--query", type=str, required=True,
                        help="Search query in natural language")
    parser.add_argument("--model", type=str, default=None,
                        help="Path to Gemma3 LiteRT model file")
    parser.add_argument("--use-llm", action="store_true",
                        help="Attempt to use LLM for parsing")
    parser.add_argument("--pretty", action="store_true",
                        help="Pretty-print JSON output")

    args = parser.parse_args()

    result = None

    if args.use_llm and args.model:
        result = llm_parse(args.query, args.model)
        if result.get("fallback"):
            result = fallback_parse(args.query)
    else:
        result = fallback_parse(args.query)

    indent = 2 if args.pretty else None
    print(json.dumps(result, indent=indent, ensure_ascii=False))


if __name__ == "__main__":
    main()
