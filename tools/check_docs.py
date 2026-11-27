"""Verify repository-relative links, language pairs, and both source maps."""

from pathlib import Path
import re
import sys
from urllib.parse import unquote, urlsplit


INLINE_LINK = re.compile(r"\]\((?:<([^>]+)>|([^\s)]+))")
HEADING = re.compile(r"^ {0,3}#{1,6}\s+(.*?)\s*#*\s*$")
FENCE = re.compile(r"^ {0,3}(`{3,}|~{3,})(.*)$")


def markdown_lines(path):
    fence = ""
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        match = FENCE.match(line)
        if fence:
            if match and match[1].startswith(fence) and not match[2].strip():
                fence = ""
            yield ""
        elif match:
            fence = match[1]
            yield ""
        else:
            yield line


def headings(lines):
    used = set()
    for line in lines:
        match = HEADING.match(line)
        if not match:
            continue
        base = re.sub(r"[^\w -]", "", match[1].lower()).replace(" ", "-")
        slug = base
        suffix = 0
        while slug in used:
            suffix += 1
            slug = f"{base}-{suffix}"
        used.add(slug)
    return used


def check(root):
    paired_documents = (
        sorted(root.glob("*.md"))
        + sorted((root / "docs").rglob("*.md"))
        + sorted((root / "examples").rglob("*.md"))
    )
    documents = (
        paired_documents
        + sorted((root / "templates").rglob("README.md"))
        + sorted((root / ".github").rglob("*.md"))
        + sorted((root / "third_party").rglob("README.md"))
    )
    contents = {doc: list(markdown_lines(doc)) for doc in documents}
    anchors = {doc: headings(lines) for doc, lines in contents.items()}
    code_docs = root / "docs" / "code"
    documented_code = {False: set(), True: set()}
    errors = []
    for doc in paired_documents:
        if doc.name.endswith(".en.md"):
            alternate = doc.name[:-len(".en.md")] + ".md"
            switch = f"[Español]({alternate})"
        else:
            alternate = f"{doc.stem}.en.md"
            switch = f"[English]({alternate})"
        if doc.with_name(alternate) not in contents or switch not in contents[doc][:8]:
            errors.append(f"{doc.relative_to(root)}: missing language switch or alternate: {alternate}")

    checked = 0
    for doc, lines in contents.items():
        in_code_reference = code_docs in doc.parents
        for number, line in enumerate(lines, start=1):
            for match in INLINE_LINK.finditer(line):
                url = match[1] or match[2]
                link = urlsplit(url)
                if link.scheme or link.netloc:
                    continue
                checked += 1
                target = (doc.parent / unquote(link.path)).resolve() if link.path else doc
                if in_code_reference and line.startswith("| [") and link.path:
                    documented_code[doc.name.endswith(".en.md")].add(target)
                location = f"{doc.relative_to(root)}:{number}"
                if root not in (target, *target.parents):
                    errors.append(f"{location}: link leaves the repository: {url}")
                elif not target.exists():
                    errors.append(f"{location}: missing target: {url}")
                elif target.suffix.lower() == ".md" and link.fragment:
                    if target not in anchors:
                        anchors[target] = headings(markdown_lines(target))
                    if unquote(link.fragment) not in anchors[target]:
                        errors.append(f"{location}: missing heading: {url}")

    sources = [
        file for folder in ("src", "include") for file in (root / folder).rglob("*")
        if file.is_file()
    ]
    for name in ("SOURCE_MAP.md", "SOURCE_MAP.en.md"):
        index = root / "docs" / name
        destinations = {
            match[1] or match[2]
            for line in contents.get(index, [])
            for match in INLINE_LINK.finditer(line)
        }
        for file in sources:
            relative = file.relative_to(root).as_posix()
            if f"../{relative}" not in destinations:
                errors.append(f"docs/{name}: missing source file: {relative}")

    code_files = set(sources)
    code_files.add(root / "CMakeLists.txt")
    code_suffixes = {".cmake", ".cpp", ".hpp", ".mm", ".py", ".ps1", ".in",
                     ".gradle", ".xml", ".plist", ".desktop", ".version", ".yml"}
    for folder in ("cmake", "packaging", "templates", "tests", "tools",
                   ".github/workflows", "examples"):
        code_files.update(
            file for file in (root / folder).rglob("*") if file.is_file()
            and (file.suffix in code_suffixes or file.name == "CMakeLists.txt")
        )
    for english, destinations in documented_code.items():
        for file in sorted(code_files - destinations):
            language = "English" if english else "Spanish"
            errors.append(f"docs/code: {language} reference missing {file.relative_to(root).as_posix()}")
    return errors, len(documents), checked, len(sources)


def main():
    root = Path(__file__).resolve().parents[1]
    errors, documents, links, sources = check(root)
    if errors:
        print("\n".join(errors), file=sys.stderr)
        return 1
    print(f"{documents} documents, {links} internal links, {sources} files in both source maps.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
