import sys
import re
from docx import Document

def ParseLexicalStream(docxPath, themeColor, outputPath="tokenstream.txt"):
    try:
        doc = Document(docxPath)
    except Exception as e:
        print(f"Error: Failed to open .docx file: {e}", file=sys.stderr)
        sys.exit(1)

    with open(outputPath, "w", encoding="utf-8") as f:
        for paragraph in doc.paragraphs:
            if not paragraph.text.strip():
                f.write("\n")
                continue

            parsedText = ParseParagraph(paragraph, themeColor)
            f.write(parsedText + "\n")

    print(f"Successfully generated {outputPath}")


def ParseParagraph(paragraph, themeColor):
    full_text = paragraph.text
    runs = paragraph.runs
    hyperlinks = paragraph.hyperlinks

    hyperlink_map = {}
    for h in hyperlinks:
        if h.text and h.url:
            hyperlink_map[h.text] = h.url

    lexeme_matches = []
    for m in re.finditer(r'\[(BT|BR):([^\]]+)\]', full_text):
        lexeme_matches.append({
            "start": m.start(),
            "end": m.end(),
            "type": m.group(1),
            "content": m.group(2),
            "full": m.group(0)
        })

    lexeme_ranges = [(lex["start"], lex["end"]) for lex in lexeme_matches]

    def is_inside_lexeme(pos):
        for s, e in lexeme_ranges:
            if s <= pos < e:
                return True
        return False

    segments = []

    for h in hyperlinks:
        if h.text and h.url:
            start = full_text.find(h.text)
            if start != -1 and not is_inside_lexeme(start):
                segments.append({
                    "start": start,
                    "end": start + len(h.text),
                    "text": h.text,
                    "url": h.url,
                    "type": "hyperlink"
                })

    for lex in lexeme_matches:
        segments.append({
            "start": lex["start"],
            "end": lex["end"],
            "full": lex["full"],
            "type": lex["type"],
            "content": lex["content"],
            "type_seg": "lexeme"
        })

    hyperlink_ranges = [(seg["start"], seg["end"]) for seg in segments if seg.get("type") == "hyperlink"]

    current_pos = 0
    for run in runs:
        text = run.text
        if not text:
            continue
        run_start = current_pos
        run_end = run_start + len(text)
        current_pos = run_end

        inside_hyperlink = False
        for hs, he in hyperlink_ranges:
            if run_start >= hs and run_end <= he:
                inside_hyperlink = True
                break
        if inside_hyperlink:
            continue

        if is_inside_lexeme(run_start):
            continue

        segments.append({
            "start": run_start,
            "end": run_end,
            "text": text,
            "run": run,
            "type_seg": "text"
        })

    segments.sort(key=lambda x: x["start"])

    result_parts = []
    for seg in segments:
        if seg.get("type") == "hyperlink":
            url = seg["url"]
            link_text = seg["text"]
            result_parts.append(f'<a target="_blank" style="color: {themeColor}; font-weight: 700;" href="{url}">{link_text}</a>')
        elif seg.get("type_seg") == "lexeme":
            lex_type = seg["type"]
            content = seg["content"]
            if lex_type == "BT":
                url = hyperlink_map.get(content)
                replacement = f"[BT:{content}:{url}]" if url else f"[BT:{content}:]"
            elif lex_type == "BR":
                buttons = [b.strip() for b in content.split('|')]
                parts = []
                for btn in buttons:
                    url = hyperlink_map.get(btn)
                    parts.append(f"{btn}:{url}" if url else f"{btn}:")
                replacement = f"[BR:{'|'.join(parts)}]"
            else:
                replacement = seg["full"]
            result_parts.append(replacement)
        else:
            run = seg["run"]
            text = seg["text"]
            if run.bold:
                text = f"<b>{text}</b>"
            if run.italic:
                text = f"<i>{text}</i>"
            if run.underline:
                text = f"<u>{text}</u>"
            if run.font.strike:
                text = f"<strike>{text}</strike>"
            result_parts.append(text)

    return "".join(result_parts)


def main():
    if len(sys.argv) < 2:
        print("Usage: python EAC_LexicalstreamParser.py <input .docx> <theme color>")
        sys.exit(1)

    docxFile = sys.argv[1]
    themeColor = sys.argv[2] if len(sys.argv) > 2 else "#185acd"

    print(f"Processing: {docxFile}")
    print(f"Theme color: {themeColor}")
    ParseLexicalStream(docxFile, themeColor)


if __name__ == "__main__":
    main()