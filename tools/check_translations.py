#!/usr/bin/env python3
"""Сверяет документы с их русскими версиями (X.md и X.ru.md).

У пары должны совпадать уровни заголовков по порядку, число блоков кода и
пометки статуса ([done], [next], [planned, ...]) по порядку: так перевод,
который забыли обновить вместе с оригиналом, не пройдёт незамеченным.
Запуск: python3 tools/check_translations.py [папка], по умолчанию docs.
"""

import re
import sys
from pathlib import Path

HEADING = re.compile(r"^(#+) ", re.MULTILINE)
STATUS = re.compile(r"\[(done|next|planned[^\]]*)\]")
FENCE = re.compile(r"^```", re.MULTILINE)


def outline(text):
    # Пометки внутри блоков кода тоже считаются: в обеих версиях код один и тот же.
    return {
        "headings": [len(level) for level in HEADING.findall(text)],
        "status marks": STATUS.findall(text),
        "code fences": len(FENCE.findall(text)),
    }


def main():
    root = Path(sys.argv[1] if len(sys.argv) > 1 else "docs")
    errors = []
    for original in sorted(root.rglob("*.md")):
        if original.name.endswith(".ru.md"):
            continue
        translation = original.with_name(original.stem + ".ru.md")
        if not translation.exists():
            errors.append(f"{original}: no Russian version {translation.name}")
            continue
        a = outline(original.read_text(encoding="utf-8"))
        b = outline(translation.read_text(encoding="utf-8"))
        for key in a:
            if a[key] != b[key]:
                errors.append(f"{original} and {translation.name}: {key} differ\n  en: {a[key]}\n  ru: {b[key]}")
    for error in errors:
        print(error, file=sys.stderr)

    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
