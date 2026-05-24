#include "queryparser.h"
#include <QRegularExpression>

const QStringList QueryParser::stopWords = {
    "un", "una", "unos", "unas", "el", "la", "los", "las", "lo",
    "de", "del", "en", "por", "para", "con", "sin", "sobre", "entre",
    "y", "e", "o", "u", "pero", "sino", "que", "como", "cual",
    "quien", "donde", "cuando", "cuanto", "se", "su", "sus",
    "le", "les", "me", "te", "nos", "os", "mi", "tu", "tu",
    "a", "ante", "bajo", "cabe", "contra", "desde", "durante",
    "hacia", "hasta", "mediante", "para", "segun", "tras", "via",
    "es", "son", "era", "fue", "sera", "seran", "esta", "estan",
    "este", "esta", "estos", "estas", "ese", "esa", "esos", "esas",
    "aquel", "aquella", "aquellos", "aquellas", "muy", "mas", "menos",
    "tambien", "tan", "tanto", "algo", "nada", "todo", "siempre",
    "nunca", "tiene", "tienen", "puede", "pueden", "hay", "hace",
    "busca", "buscar", "encuentra", "encontrar", "necesito",
    "quiero", "podrias", "puedes", "ayuda", "ayudar", "archivo",
    "archivos", "fichero", "ficheros", "carpeta", "carpetas",
    "directorio", "directorios", "ubicado", "llamado", "nombre",
    "tipo", "tipos", "donde", "cual", "algun", "alguna", "algunos",
    "algunas", "ver", "mostrar", "abrir", "tengo", "tener",
    "please", "find", "search", "look", "for", "the", "a", "an",
    "in", "at", "with", "and", "or", "of", "to", "is", "are",
    "was", "were", "will", "can", "could", "would", "should",
    "have", "has", "had", "do", "does", "did", "this", "that",
    "these", "those", "i", "my", "me", "you", "your", "it", "its",
    "we", "our", "they", "them", "their", "file", "folder",
    "directory", "called", "named", "show", "open", "need", "want"
};

const QVector<QPair<QString, QStringList>> QueryParser::categoryMap = {
    {"image", {"foto", "fotografia", "fotografía", "imagen", "imagenes", "imágenes",
               "captura", "pantalla", "screenshot", "fotos", "photo", "photos",
               "picture", "pictures", "image", "images", "fotico", "grafico",
               "gráfico", "dibujo", "ilustración", "ilustracion"}},
    {"document", {"documento", "documentos", "document", "documents", "pdf",
                  "word", "texto", "txt", "archivo", "hoja", "calculo",
                  "cálculo", "excel", "presentacion", "presentación",
                  "powerpoint", "diapositiva", "slide", "spreadsheet",
                  "hoja de calculo", "hoja de cálculo", "lectura",
                  "libro", "novela", "articulo", "artículo", "escrito",
                  "carta", "formulario", "informe", "reporte"}},
    {"video", {"video", "videos", "vídeo", "vídeos", "pelicula", "película",
               "movie", "movies", "film", "films", "clip", "grabacion",
               "grabación", "corto", "corto", "documental", "serie",
               "episodio", "animacion", "animación"}},
    {"audio", {"audio", "audios", "musica", "música", "music", "song",
               "cancion", "canción", "canciones", "podcast", "grabacion",
               "grabación", "sonido", "sound", "voz", "voice", "album",
               "álbum", "playlist", "lista"}}
};

ParsedQuery QueryParser::parse(const QString &query) {
    ParsedQuery result;
    result.originalQuery = query;

    if (query.trimmed().isEmpty()) return result;

    QString queryLower = query.toLower();
    result.searchContent = queryLower.contains("dice") ||
                           queryLower.contains("contiene") ||
                           queryLower.contains("contenga") ||
                           queryLower.contains("menciona") ||
                           queryLower.contains("habla") ||
                           queryLower.contains("texto") ||
                           queryLower.contains("digan") ||
                           queryLower.contains("diga") ||
                           queryLower.contains("contengan");

    result.fileCategory = detectFileCategory(queryLower);
    result.fileExtensions = detectExtensions(result.fileCategory);
    result.exactPhrases = extractExactPhrases(queryLower);
    result.keywords = extractKeywords(queryLower);

    return result;
}

QStringList QueryParser::extractKeywords(const QString &query) {
    QStringList words;
    QString queryLower = query.toLower();

    QRegularExpression nonAlpha(R"([^a-záéíóúüñA-ZÁÉÍÓÚÜÑ0-9])");
    QStringList parts = queryLower.split(nonAlpha, Qt::SkipEmptyParts);

    for (const QString &word : parts) {
        QString cleaned = cleanWord(word);
        if (cleaned.length() < 2) continue;
        if (stopWords.contains(cleaned)) continue;

        bool isCategory = false;
        for (const auto &cat : categoryMap) {
            if (cat.second.contains(cleaned)) {
                isCategory = true;
                break;
            }
        }
        if (isCategory) continue;

        if (!words.contains(cleaned))
            words.append(cleaned);
    }

    return words;
}

QString QueryParser::detectFileCategory(const QString &query) {
    for (const auto &cat : categoryMap) {
        for (const QString &word : cat.second) {
            if (query.contains(word)) {
                return cat.first;
            }
        }
    }
    return "all";
}

QStringList QueryParser::detectExtensions(const QString &category) {
    if (category == "image") {
        return {"jpg", "jpeg", "png", "gif", "bmp", "webp", "svg",
                "tiff", "tif", "ico", "heic", "heif", "raw", "cr2", "nef"};
    }
    if (category == "document") {
        return {"pdf", "doc", "docx", "xls", "xlsx", "ppt", "pptx",
                "odt", "ods", "odp", "txt", "rtf", "csv", "md",
                "tex", "epub", "mobi", "pages", "numbers", "key"};
    }
    if (category == "video") {
        return {"mp4", "avi", "mkv", "mov", "wmv", "flv", "webm",
                "m4v", "mpg", "mpeg", "3gp", "ogv", "ts", "mts"};
    }
    if (category == "audio") {
        return {"mp3", "wav", "flac", "aac", "ogg", "wma", "m4a",
                "opus", "mid", "midi", "aiff", "alac"};
    }
    return {};
}

QStringList QueryParser::extractExactPhrases(const QString &query) {
    QStringList phrases;
    QRegularExpression quoteRe(R"(["“”]([^"“”]+)["“”])");
    auto it = quoteRe.globalMatch(query);
    while (it.hasNext()) {
        auto match = it.next();
        phrases.append(match.captured(1).trimmed().toLower());
    }
    return phrases;
}

QString QueryParser::cleanWord(const QString &word) {
    QString w = word.toLower();
    w.remove(QRegularExpression(R"([^a-záéíóúüñ0-9])"));
    return w;
}
