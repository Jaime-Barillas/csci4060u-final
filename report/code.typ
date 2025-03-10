#let anchor-regex = regex(".+ANCHOR(?:_END)?:\s*([\w_-]+)")
#let anchor-start = regex("ANCHOR:")
#let cache        = state("code.insert.cache", (:))

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
    section.insert("text", text.trim())
    sections.insert(anchor-name, section)
  }

  return sections
}

#let insert(file-path, anchor: "<anchor-name>", lang: "c", wrap-in-figure: true, caption: none) = {
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

  let raw-code = context {
    let section = cache.get().at(file-path, default: (:)).at(anchor, default: (:))
    if "text" not in section {
      panic("This shouldn't happen! `text` not found in section dict for " + anchor + " of " + file-path)
    }

    let code = raw(section.at("text", default: none), lang: lang)
    if wrap-in-figure {
      return figure(code, caption: caption)
    } else {
      return code
    }
  }
  if raw-code == none {
    panic("Unable to find anchor " + anchor + " in file " + file-path)
  }

  // NOTE: `return raw-code` does not evaluate the update function above and
  //       triggers the panic() in the contex block.
  raw-code
}

