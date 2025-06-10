use "itertools"

primitive Format
  fun msg(fmt: String box, args: Array[Stringable] box): String iso^ =>
    let strs = Iter[String](fmt.split_by("{}").values())
    let vals = Iter[Stringable](args.values())

    let res = strs
      .filter_map[Stringable]({(str) => if str.size() != 0 then str end})
      .interleave(vals)
      .filter({(it) => it isnt None})

    "".join(res)
