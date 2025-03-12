#let anchor-regex     = regex(".+ANCHOR(?:_END)?:\s*([\w_-]+)")
#let anchor-start     = regex("ANCHOR:")
#let line-end-regex   = regex("\r?\n")
#let leading-ws-regex = regex("(?-m)^\s*") // Ensure no multi-line mode, match at start of string.
#let cache = state("code.insert.cache", (:))

#let trim-whitespace(text) = {
  // Strip leading whitespace on each line to ensure consistent indentation.
  // NOTE: The underlying regex engine does not support look-around. https://docs.rs/regex/latest/regex/
  // 1. Determine the number of spaces or tabs leading the first non-whitespace
  //    character on the first line.
  // 2. Create a regex that matches that number of whitespaces.
  // 3. Trim surrounding whitespace.
  // 4. Replace leading whitespace on each line with the empty string.
  let leading-ws-count = text.trim(line-end-regex).find(leading-ws-regex).len()
  let n-leading-ws-regex = regex("(?m)^\s{" + str(leading-ws-count) + "}")

  return text.trim().replace(n-leading-ws-regex, "")
}

#let extract-all-sections(file-path) = {
  // We want to handle nested anchors so we need to manually extract code
  // within two anchors.
  // 1. First, extract the positions and names of all anchors.
  // 2. For each match:
  //    a) If it is a start anchor:
  //       + If a section dict with the same anchor name exists:
  //         - Panic (redefining an open anchor).
  //           TODO: Is there a way to warn instead of panicing?
  //       + Create a section dict keyed by the anchor name.
  //       + Save the end position + 1 as the section start position.
  //    b) If it is an end anchor:
  //       + If no section dict with the same anchor name exists:
  //         - Panic (closing an unopened anchor).
  //       + If the corresponding section dict already has an end position:
  //         - Panic (redefining a closing anchor).
  //       + Save the start position as the section end position.
  // 3. For each section:
  //    a) If it has no end position:
  //       + Panic (unclosed anchor).
  //    b) Extract the text between the start and end position.
  //    c) Trim whitespace.
  // 4. Return the dictionary of sections.

  let sections = (:)
  let source   = read(file-path)
  let matches  = source.matches(anchor-regex)
  for match in matches {
    let anchor-name = match.captures.first()
    if match.text.contains(anchor-start) {
      // Start anchor.
      if anchor-name in sections {
        panic("Redefining open anchor " + anchor-name + " in " + file-path)
      }

      sections.insert(anchor-name, (start: match.end + 1))
    } else {
      // End anchor.
      if anchor-name not in sections {
        panic("Closing an unopened anchor " + anchor-name + " in " + file-path)
      }
      if "end" in sections.at(anchor-name) {
        panic("Redefining closing anchor " + anchor-name + " in " + file-path)
      }

      let section = sections.at(anchor-name)
      section.insert("end", match.start)
      sections.insert(anchor-name, section)
    }
  }

  for (anchor-name, section) in sections {
    if "end" not in section {
      panic("Unclosed anchor " + anchor-name + " in " + file-path)
    }

    let text = source.slice(section.start, section.end)
    section.insert("text", trim-whitespace(text))
    sections.insert(anchor-name, section)
  }

  return sections
}

/// Inserts the contents of an anchor section from the file located at
/// `file-path`. Attaches a label keyed by `<file-path>:<anchor>` with
/// all leading '../' removed and intermediate '/' replaced with ':'
/// (`wrap-in-figure` must be true). Anchor sections are delimited with
/// `ANCHOR: <name>` and `ANCHOR_END: <name>` in comments. Anchors can be
/// nested (inner anchor comments will be included in the extracted code,
/// TODO: Strip out anchor comment lines).
#let insert(
  /// Path to the file containing the anchor section to insert.
  /// -> str
  file-path,
  /// The name of the specific anchor to insert.
  /// -> str
  anchor,
  /// Overriding name of the generated label.
  /// -> str
  label-key: none,
  /// The language of the inserted code.
  /// -> str
  lang: "c",
  /// Whether the code should be wrapped in a figure. Must be true to generate
  /// a label.
  /// -> bool
  wrap-in-figure: true,
  /// An optional caption for the code figure. Only takes effect if
  /// `wrap-in-figure` is true.
  caption: none) = {
  // Insert does 3 things:
  // 1. Process and cache file if necessary.
  // 2. Extract the desired anchor section.
  // 3. Wrap extracted section in a raw code block.

  // Process `file-path` if necessary.
  cache.update(prev => {
    if file-path in prev {
      return prev
    }

    let sections = extract-all-sections(file-path)
    prev.insert(file-path, sections)

    return prev
  })

  // FIXME: Better label-key default. Handle paths like: test/../code.c
  //        Better to wait until typst has a Path type so edge cases are easier
  //        to handle?
  if label-key == none {
    label-key = file-path.replace("../", "").replace("/", ":") + ":" + anchor
  }
  let raw-code = context {
    let section = cache.get().at(file-path, default: (:)).at(anchor, default: (:))
    if "text" not in section {
      panic("This shouldn't happen! `text` not found in section dict for " + anchor + " of " + file-path)
    }

    let code = raw(section.at("text", default: none), lang: lang, block: true, align: left)
    if wrap-in-figure {
      // Labels can only be attached in markup mode, not code mode. We wrap the
      // figure in [] to switch to markup mode and apply the label there.
      // See last sentence of: https://typst.app/docs/reference/foundations/label/#syntax
      [#figure(code, caption: caption) #label(label-key)]
    } else {
      code
    }
  }
  if raw-code == none {
    panic("Unable to find anchor " + anchor + " in file " + file-path)
  }

  // NOTE: `return raw-code` does not evaluate the update function above and
  //       triggers the panic() in the contex block.
  raw-code
}

