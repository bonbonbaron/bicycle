# bicycle for Blender: place genomes in a 3D level and export it (docs/blender-entities.md has the clicks).
#   Install: Edit > Preferences > Add-ons > (the down-arrow menu, top right) Install from Disk... > this file.
#   Use:     3D Viewport > Sidebar (N key) > Bicycle tab.
#   Headless: blender -b <game>/blender/<x>.blend --python scripts/blender/export.py   (exports every scene)
# A Blender scene is one level: genome/level/<level>.lua gets its meshes (role "static"/"decor"/"collider", as the
# bicycle_role custom property says), the entities placed in it (Empties tagged with a genome, with that genome's
# per-instance parameters as drop-downs and fields) and its spawn points. Genome models (bicycle_role "character")
# go to genome/body/model/. The engine (scripts/Scene.lua) spawns the entities when the scene loads.
bl_info = {
    "name": "bicycle: entities and level export",
    "author": "Michael Baucum",
    "version": (1, 0, 0),
    "blender": (4, 2, 0),
    "location": "3D Viewport > Sidebar (N) > Bicycle",
    "description": "Place a bicycle game's genomes (drop-downs from its genome folder) and export the level",
    "category": "Object",
}

import re

# ---- a tolerant reader of the data in bicycle's Lua files (plain Python, no Blender needed) ----------------------
# Reads table constructors, strings, numbers, booleans, `local x = ...` / `x.y = ...` assignments and `return ...`;
# functions and other code are skipped. Enough for genomes, scenes and level files, whose data is literal.
_TOKEN = re.compile(r"""
    (?P<ws>\s+|--\[(?P<eq>=*)\[.*?\](?P=eq)\]|--[^\n]*)
  | (?P<lstr>\[(?P<eq2>=*)\[.*?\](?P=eq2)\])
  | (?P<str>"(?:\\.|[^"\\\n])*"|'(?:\\.|[^'\\\n])*')
  | (?P<num>0[xX][0-9a-fA-F]+|(?:\d+\.?\d*|\.\d+)(?:[eE][+-]?\d+)?)
  | (?P<name>[A-Za-z_][A-Za-z_0-9]*)
  | (?P<op>\.\.\.|\.\.|==|~=|<=|>=|::|[-+*/%^\#&~|<>=(){}\[\];:,.])
""", re.S | re.X)

_ESC = {"n": "\n", "t": "\t", "r": "\r", "\\": "\\", '"': '"', "'": "'", "a": "\a", "b": "\b", "f": "\f", "v": "\v",
        "0": "\0", "\n": "\n"}


def _unquote(s):
    out, i, body = [], 0, s[1:-1]
    while i < len(body):
        c = body[i]
        if c == "\\" and i + 1 < len(body):
            out.append(_ESC.get(body[i + 1], body[i + 1]))
            i += 2
        else:
            out.append(c)
            i += 1
    return "".join(out)


def tokenize(src):
    toks, pos = [], 0
    while pos < len(src):
        m = _TOKEN.match(src, pos)
        if not m:
            toks.append(("op", src[pos]))
            pos += 1
            continue
        pos = m.end()
        k = m.lastgroup
        if m.group("ws") is not None:
            continue
        if m.group("lstr") is not None:
            v = m.group("lstr")
            n = v.index("[", 1) + 1
            toks.append(("str", v[n:len(v) - n].lstrip("\n")))
        elif m.group("str") is not None:
            toks.append(("str", _unquote(m.group("str"))))
        elif m.group("num") is not None:
            t = m.group("num")
            v = int(t, 16) if t[:2].lower() == "0x" else float(t)
            toks.append(("num", int(v) if isinstance(v, float) and v.is_integer() and "." not in t and "e" not in t.lower() else v))
        elif m.group("name") is not None:
            toks.append(("name", m.group("name")))
        else:
            toks.append(("op", m.group("op")))
    toks.append(("eof", None))
    return toks


class _Unknown:
    """A value the reader could not evaluate (a function call, an expression on variables...)."""
    def __repr__(self):
        return "<?>"


UNKNOWN = _Unknown()
_BLOCK_OPEN = {"function", "do", "then", "repeat"}
_BLOCK_CLOSE = {"end", "until"}
_KEYWORDS = {"and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if", "in", "local",
             "nil", "not", "or", "repeat", "return", "then", "true", "until", "while"}


class Reader:
    def __init__(self, src):
        self.t, self.i, self.env, self.result = tokenize(src), 0, {}, None

    def peek(self, k=0):
        return self.t[min(self.i + k, len(self.t) - 1)]

    def next(self):
        tok = self.t[self.i]
        self.i = min(self.i + 1, len(self.t) - 1)
        return tok

    def accept(self, kind, val=None):
        k, v = self.peek()
        if k == kind and (val is None or v == val):
            self.i += 1
            return True
        return False

    def skip_block(self):  # after a block opener: to its matching end/until
        depth = 1
        while depth and self.peek()[0] != "eof":
            k, v = self.next()
            if k == "name" and v in ("function", "do", "repeat"):
                depth += 1
            elif k == "name" and v == "if":
                depth += 1
                self._skip_to("then")
            elif k == "name" and v in ("while", "for"):
                depth += 1
                self._skip_to("do")
            elif k == "name" and v in _BLOCK_CLOSE:
                depth -= 1

    def _skip_to(self, word):
        while self.peek()[0] != "eof" and self.peek() != ("name", word):
            self.next()
        self.next()

    def skip_balanced(self, open_, close):
        depth = 1
        while depth and self.peek()[0] != "eof":
            k, v = self.next()
            if k == "op" and v == open_:
                depth += 1
            elif k == "op" and v == close:
                depth -= 1

    # expressions: literals and tables are evaluated; anything else becomes UNKNOWN (and is skipped)
    def expr(self):
        v = self.simple()
        while True:
            k, op = self.peek()
            if k == "op" and op in ("+", "-", "*", "/", "..", "%", "^", "==", "~=", "<", ">", "<=", ">=") or \
               k == "name" and op in ("and", "or"):
                self.next()
                w = self.simple()
                try:
                    if op == "..":
                        v = _lstr(v) + _lstr(w)
                    elif op in ("+", "-", "*", "/") and all(isinstance(x, (int, float)) for x in (v, w)):
                        v = {"+": v + w, "-": v - w, "*": v * w, "/": v / w if w else UNKNOWN}[op]
                    elif op == "or":
                        v = v if v not in (None, False, UNKNOWN) else w
                    else:
                        v = UNKNOWN
                except TypeError:
                    v = UNKNOWN
            else:
                return v

    def simple(self):
        k, v = self.next()
        if k in ("str", "num"):
            return self.suffix(v)
        if k == "op" and v == "{":
            return self.suffix(self.table())
        if k == "op" and v == "-":
            w = self.simple()
            return -w if isinstance(w, (int, float)) else UNKNOWN
        if k == "op" and v == "#":
            w = self.simple()
            return len(w) if isinstance(w, (list, str)) else UNKNOWN
        if k == "op" and v == "(":
            w = self.expr()
            self.accept("op", ")")
            return self.suffix(w)
        if k == "name":
            if v == "true":
                return True
            if v == "false":
                return False
            if v == "nil":
                return None
            if v == "not":
                self.simple()
                return UNKNOWN
            if v == "function":
                self.skip_balanced("(", ")") if self.accept("op", "(") else None
                self.skip_block()
                return UNKNOWN
            return self.suffix(self.env.get(v, UNKNOWN))
        if k == "op" and v == "...":
            return UNKNOWN
        return UNKNOWN

    def suffix(self, v):
        while True:
            k, op = self.peek()
            if k == "op" and op == "." and self.peek(1)[0] == "name":
                self.next()
                name = self.next()[1]
                v = v.get(name, UNKNOWN) if isinstance(v, dict) else UNKNOWN
            elif k == "op" and op == "[":
                self.next()
                key = self.expr()
                self.accept("op", "]")
                v = _index_of(v, key)
            elif k == "op" and op in (":",) and self.peek(1)[0] == "name":
                self.next()
                self.next()
            elif k == "op" and op == "(":
                self.next()
                self.skip_balanced("(", ")")
                v = UNKNOWN
            elif k in ("str",) or (k == "op" and op == "{" and v is UNKNOWN):
                self.simple()  # f"x" / f{...} call sugar
                v = UNKNOWN
            else:
                return v

    def table(self):
        arr, rec = [], {}
        while not self.accept("op", "}"):
            if self.peek()[0] == "eof":
                break
            if self.peek() == ("op", "["):
                self.next()
                key = self.expr()
                self.accept("op", "]")
                self.accept("op", "=")
                val = self.expr()
                if isinstance(key, int) and key >= 1 and not isinstance(key, bool):
                    rec[key] = val
                elif isinstance(key, (str, float, int)):
                    rec[key] = val
            elif self.peek()[0] == "name" and self.peek(1) == ("op", "=") and self.peek()[1] not in _KEYWORDS:
                key = self.next()[1]
                self.next()
                rec[key] = self.expr()
            else:
                arr.append(self.expr())
            if not (self.accept("op", ",") or self.accept("op", ";")):
                self.accept("op", "}")
                break
        if not rec:
            return arr if arr else {}
        for n, v in enumerate(arr, 1):
            rec[n] = v
        return rec

    # statements
    def run(self):
        while self.peek()[0] != "eof":
            self.statement()
        return self.result

    def statement(self):
        k, v = self.peek()
        if k == "name" and v == "return":
            self.next()
            if self.peek()[0] != "eof" and self.peek() != ("name", "end"):
                r = self.expr()
                if self.result is None:
                    self.result = r
            return
        if k == "name" and v == "local":
            self.next()
            if self.accept("name", "function"):
                self.next()
                self.accept("op", "(") and self.skip_balanced("(", ")")
                self.skip_block()
                return
            names = [self.next()[1]]
            while self.accept("op", ","):
                names.append(self.next()[1])
            if self.peek() == ("op", "<"):  # <const>
                self.next(); self.next(); self.accept("op", ">")
            if self.accept("op", "="):
                vals = [self.expr()]
                while self.accept("op", ","):
                    vals.append(self.expr())
                for n, name in enumerate(names):
                    self.env[name] = vals[n] if n < len(vals) else None
            return
        if k == "name" and v == "function":
            self.next()
            while self.peek()[0] == "name" or self.peek() in (("op", "."), ("op", ":")):
                self.next()
            self.accept("op", "(") and self.skip_balanced("(", ")")
            self.skip_block()
            return
        if k == "name" and v in ("if", "while", "for", "do", "repeat"):
            self.next()
            if v in ("if",):
                self._skip_to("then")
            elif v in ("while", "for"):
                self._skip_to("do")
            self.skip_block()
            return
        if k == "name" and v not in _KEYWORDS:
            # assignment target: name(.field | [key])*
            self.next()
            target, path = v, []
            while True:
                if self.peek() == ("op", ".") and self.peek(1)[0] == "name":
                    self.next()
                    path.append(self.next()[1])
                elif self.peek() == ("op", "["):
                    self.next()
                    path.append(self.expr())
                    self.accept("op", "]")
                elif self.peek()[0] == "op" and self.peek()[1] in ("(", ":") or self.peek()[0] == "str" or \
                        self.peek() == ("op", "{"):
                    self.suffix(UNKNOWN)  # a call statement
                    return
                else:
                    break
            if self.accept("op", "="):
                val = self.expr()
                if not path:
                    self.env[target] = val
                else:
                    t = self.env.get(target)
                    for p in path[:-1]:
                        t = _index_of(t, p)
                    if isinstance(t, dict):
                        t[path[-1]] = val
            return
        self.next()  # anything else (`;`, `end` of a skipped block...)


def _lstr(v):
    if isinstance(v, str):
        return v
    if isinstance(v, (int, float)) and not isinstance(v, bool):
        return num(v)
    return "<?>"


def _index_of(t, key):
    if isinstance(t, dict):
        return t.get(key, UNKNOWN)
    if isinstance(t, list) and isinstance(key, int) and 1 <= key <= len(t):
        return t[key - 1]
    return UNKNOWN


def read(src):
    """The value a Lua chunk returns (dicts, lists, str, int/float, bool); None if it returns nothing readable."""
    try:
        return Reader(src).run()
    except (IndexError, RecursionError):
        return None


def read_file(path):
    try:
        with open(path, encoding="utf-8", errors="replace") as f:
            return read(f.read())
    except OSError:
        return None


def as_list(t):
    if isinstance(t, list):
        return t
    if isinstance(t, dict):
        return [t[i] for i in range(1, len(t) + 1) if i in t]
    return []


def dig(t, dotted):
    for p in dotted.split("."):
        t = t.get(p) if isinstance(t, dict) else None
    return t


# writer: Lua source of a value (sorted keys, so exports diff cleanly)
def num(v):
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, int):
        return str(v)
    v = round(float(v), 4)
    return str(int(v)) if v == int(v) else repr(v)


_IDENT = re.compile(r"^[A-Za-z_][A-Za-z_0-9]*$")


def _key(k):
    if isinstance(k, str) and _IDENT.match(k) and k not in _KEYWORDS:
        return k
    return "[%s]" % lua(k)


def lua(v):
    if v is None:
        return "nil"
    if isinstance(v, bool) or isinstance(v, (int, float)):
        return num(v)
    if isinstance(v, str):
        return '"%s"' % v.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")
    if isinstance(v, (list, tuple)):
        return "{ %s }" % ", ".join(lua(x) for x in v) if v else "{}"
    if isinstance(v, dict):
        if not v:
            return "{}"
        order = sorted(v, key=lambda k: (k != "genome", k != "name", str(k)))
        return "{ %s }" % ", ".join("%s = %s" % (_key(k), lua(v[k])) for k in order)
    return "nil"


# =====================================================================================================================
# Blender add-on (everything below needs Blender)
# =====================================================================================================================
import math
import os
import struct

try:
    import bpy
    import bmesh
    from bpy.props import (BoolProperty, CollectionProperty, EnumProperty, FloatProperty, PointerProperty,
                           StringProperty)
    from mathutils import Matrix, Vector
except ImportError:  # plain Python (the Lua reader's tests)
    bpy = None

# Param types a genome's `params` table can give (see docs/blender-entities.md):
#   "scene"            a drop-down of the game's scenes
#   "spawn:<param>"    a drop-down of the spawn points of the scene the <param> param names (e.g. "spawn:door.to")
#   "genome"           a drop-down of the game's genomes
#   "enum:a|b|c"       a drop-down of those values
#   "number", "int", "bool", "string"
#   "path"             points: the entity's child path points (Add path point) or a child curve, exported relative
#                      to the entity, starting at { 0, 0, 0 }

CHOICES_CACHE = {}  # keeps EnumProperty item lists alive (Blender needs the strings to outlive the callback)


# ---- the game ---------------------------------------------------------------------------------------------------------
def genome_dir(scene=None):
    """The game's genome folder: the scene's setting, else <game>/genome for a .blend in <game>/blender/."""
    scene = scene or bpy.context.scene
    d = scene.bicycle.genome_dir if scene and hasattr(scene, "bicycle") else ""
    if d:
        return os.path.abspath(bpy.path.abspath(d))
    blend = bpy.data.filepath
    if blend:
        game = os.path.dirname(os.path.dirname(os.path.abspath(blend)))
        if os.path.isdir(os.path.join(game, "genome")):
            return os.path.join(game, "genome")
    return ""


class Catalog:
    """What the game has: genomes (with their parsed tables and param schemas), scenes and their spawn points."""
    def __init__(self, gdir):
        self.dir = gdir
        self.genomes, self.scenes, self.spawns, self.levels = {}, [], {}, {}
        if not gdir or not os.path.isdir(gdir):
            return
        for f in sorted(os.listdir(gdir)):
            if not f.endswith(".lua"):
                continue
            t = read_file(os.path.join(gdir, f))
            t = t if isinstance(t, dict) else {}
            if isinstance(t.get("scene"), str):  # a starter genome that only names a scene (test's intro.lua)
                continue
            self.genomes[f[:-4]] = t
        sdir = os.path.join(gdir, "scene")
        for f in sorted(os.listdir(sdir)) if os.path.isdir(sdir) else []:
            if not f.endswith(".lua"):
                continue
            name = f[:-4]
            self.scenes.append(name)
            t = read_file(os.path.join(sdir, f))
            t = t if isinstance(t, dict) else {}
            names = set(t.get("spawns") or {}) if isinstance(t.get("spawns"), dict) else set()
            w = t.get("world3d") if isinstance(t.get("world3d"), dict) else {}
            if isinstance(w.get("spawns"), dict):
                names |= set(w["spawns"])
            level = w.get("level")
            if isinstance(level, str):
                level = os.path.splitext(os.path.basename(level))[0]
                self.levels[name] = level
                lt = read_file(os.path.join(gdir, "level", level + ".lua"))
                if isinstance(lt, dict) and isinstance(lt.get("spawns"), dict):
                    names |= set(lt["spawns"])
            self.spawns[name] = names

    def params(self, genome):
        """[(name, type)] of a genome's params table, in a stable order."""
        p = self.genomes.get(genome, {}).get("params")
        if not isinstance(p, dict):
            return []
        return sorted(((k, v) for k, v in p.items() if isinstance(k, str) and isinstance(v, str)), key=lambda kv: kv[0])

    def spawn_names(self, scene):
        names = set(self.spawns.get(scene, set()))
        level = self.levels.get(scene, scene)
        for s in bpy.data.scenes:  # spawn points placed in this .blend but not exported yet
            if level_name(s) == level:
                for o in s.objects:
                    if o.bicycle.kind == "SPAWN" and o.bicycle.spawn:
                        names.add(o.bicycle.spawn)
        names.add("default")
        return sorted(names, key=lambda n: (n != "default", n))


_catalog = {"dir": None, "cat": None}


def catalog(refresh=False):
    gdir = genome_dir()
    if refresh or _catalog["dir"] != gdir or _catalog["cat"] is None:
        _catalog["dir"], _catalog["cat"] = gdir, Catalog(gdir)
    return _catalog["cat"]


def level_name(scene):
    """The level file a Blender scene exports to (genome/level/<name>.lua)."""
    n = scene.bicycle.level.strip() if hasattr(scene, "bicycle") else ""
    if n:
        return n
    return "scene3d" if scene.name == "Scene" else scene.name


# ---- enum items ---------------------------------------------------------------------------------------------------
def _items(key, values, blank=None):
    items = [(v, v, "") for v in values]
    if blank is not None:
        items.insert(0, ("", blank, ""))
    if not items:
        items = [("", "(none)", "")]
    CHOICES_CACHE[key] = items
    return items


def _index(items, value):
    for i, it in enumerate(items):
        if it[0] == value:
            return i
    return 0


def genome_items(self, context):
    return _items("genomes", sorted(catalog().genomes))


def param_items(self, context):
    cat = catalog()
    t = self.ptype
    if t == "scene":
        return _items("p:" + self.as_pointer().__str__(), cat.scenes, blank="(choose a scene)")
    if t.startswith("spawn"):
        of = t.split(":", 1)[1] if ":" in t else ""
        owner = self.id_data
        scene = ""
        for p in owner.bicycle.params:
            if p.name == of:
                scene = p.value
        names = cat.spawn_names(scene) if scene else sorted({n for s in cat.scenes for n in cat.spawn_names(s)})
        return _items("p:" + str(self.as_pointer()), names)
    if t == "genome":
        return _items("p:" + str(self.as_pointer()), sorted(cat.genomes), blank="(none)")
    if t.startswith("enum:"):
        return _items("p:" + str(self.as_pointer()), t[5:].split("|"))
    return _items("p:" + str(self.as_pointer()), [])


def _choice_get(self):
    return _index(param_items(self, None), self.value)


def _choice_set(self, i):
    items = param_items(self, None)
    if 0 <= i < len(items):
        self.value = items[i][0]


def _number_get(self):
    try:
        return float(self.value)
    except ValueError:
        return 0.0


def _number_set(self, v):
    self.value = num(int(round(v)) if self.ptype == "int" else v)


def _flag_get(self):
    return self.value == "true"


def _flag_set(self, v):
    self.value = "true" if v else "false"


def _genome_get(self):
    return _index(genome_items(self, None), self.genome)


def _genome_set(self, i):
    items = genome_items(self, None)
    if 0 <= i < len(items):
        set_genome(self.id_data, items[i][0])


def _add_genome_get(self):
    return _index(genome_items(self, None), self.add_genome)


def _add_genome_set(self, i):
    items = genome_items(self, None)
    if 0 <= i < len(items):
        self.add_genome = items[i][0]


def _encode(v):
    if isinstance(v, bool):
        return "true" if v else "false"
    if isinstance(v, (int, float)):
        return num(v)
    if isinstance(v, str):
        return v
    return ""


if bpy:
    class BicycleParam(bpy.types.PropertyGroup):
        # name (inherited): the param's dotted field, e.g. "door.to"
        ptype: StringProperty(name="Type")
        value: StringProperty(name="Value")
        default: StringProperty(name="Default")
        choice: EnumProperty(name="Value", items=param_items, get=_choice_get, set=_choice_set)
        number: FloatProperty(name="Value", get=_number_get, set=_number_set)
        flag: BoolProperty(name="Value", get=_flag_get, set=_flag_set)

    class BicycleObject(bpy.types.PropertyGroup):
        kind: EnumProperty(name="Kind", items=[("NONE", "None", ""), ("ENTITY", "Entity", "A placed genome"),
                                                ("SPAWN", "Spawn point", "Where the player arrives"),
                                                ("PATH", "Path point", "A point of its parent entity's path")])
        genome: StringProperty(name="Genome")
        genome_choice: EnumProperty(name="Genome", items=genome_items, get=_genome_get, set=_genome_set,
                                    description="The genome (genome/<name>.lua) this entity is an instance of")
        params: CollectionProperty(type=BicycleParam)
        spawn: StringProperty(name="Spawn name", description="Doors name it as their spawn")
        persist: BoolProperty(name="Persists", description="Carried to the next scene (persist = true)")

    class BicycleScene(bpy.types.PropertyGroup):
        level: StringProperty(name="Level", description="genome/level/<name>.lua; empty: the Blender scene's name")
        genome_dir: StringProperty(name="Genome folder", subtype="DIR_PATH",
                                   description="The game's genome folder; empty: <game>/genome next to <game>/blender/")
        add_genome: StringProperty(name="Genome")
        add_genome_choice: EnumProperty(name="Genome", items=genome_items, get=_add_genome_get, set=_add_genome_set)
        spawn_name: StringProperty(name="Name", default="default")
        export_level: BoolProperty(name="Is a level", default=True,
                                   description="Off for a scene that only holds genome models (exports their meshes)")


# ---- placing ----------------------------------------------------------------------------------------------------------
def set_genome(obj, genome):
    """Makes obj an instance of genome: its param list (keeping values of params it had) and its preview."""
    b = obj.bicycle
    b.kind, b.genome = "ENTITY", genome
    cat = catalog()
    old = {p.name: p.value for p in b.params}
    b.params.clear()
    table = cat.genomes.get(genome, {})
    for name, ptype in cat.params(genome):
        p = b.params.add()
        p.name, p.ptype = name, ptype
        p.default = _encode(dig(table, name))
        p.value = old.get(name, p.default)
    preview(obj, table)


def preview(obj, table):
    """Shows the genome's model on its Empty (an instanced collection holding the model), or a marker."""
    model = table.get("model") if isinstance(table.get("model"), str) else None
    obj.empty_display_size = 0.4
    obj.empty_display_type = "PLAIN_AXES"
    door = table.get("door") if isinstance(table.get("door"), dict) else None
    if door:
        obj.empty_display_type = "SPHERE"
        r = door.get("radius")
        obj.empty_display_size = float(r) if isinstance(r, (int, float)) else 1.0
    coll = preview_collection(model) if model else None
    obj.instance_type = "COLLECTION" if coll else "NONE"
    obj.instance_collection = coll


def preview_collection(model):
    name = "bicycle preview: " + model
    coll = bpy.data.collections.get(name)
    if coll:
        return coll
    src = bpy.data.objects.get(model)
    objs = []
    if src and src.type == "MESH":
        objs = [src]  # the model is authored in this .blend
    else:
        path = os.path.join(genome_dir(), "body", "model", model + ".obj")
        if not os.path.exists(path):
            return None
        before = set(bpy.data.objects)
        bpy.ops.wm.obj_import(filepath=path, forward_axis="NEGATIVE_Z", up_axis="Y")
        objs = [o for o in bpy.data.objects if o not in before]
        for o in objs:
            for c in list(o.users_collection):
                c.objects.unlink(o)
            o["bicycle_role"] = "preview"
    coll = bpy.data.collections.new(name)
    for o in objs:
        coll.objects.link(o)
    return coll


def add_entity(scene, genome, location=(0, 0, 0), params=None, name=None, rotation_z=0.0, persist=False):
    """Places a genome: an Empty tagged with it (Python API of the Add entity button)."""
    obj = bpy.data.objects.new(name or genome, None)
    scene.collection.objects.link(obj)
    obj.location = location
    obj.rotation_euler = (0, 0, rotation_z)
    set_genome(obj, genome)
    obj.bicycle.persist = persist
    for k, v in (params or {}).items():
        p = next((p for p in obj.bicycle.params if p.name == k), None)
        if p is None:
            raise KeyError("genome %s has no param %s (its params: %s)" % (genome, k, ", ".join(q.name for q in obj.bicycle.params)))
        p.value = _encode(v)
    return obj


def add_spawn(scene, name, location=(0, 0, 0), rotation_z=0.0):
    obj = bpy.data.objects.new("spawn " + name, None)
    scene.collection.objects.link(obj)
    obj.location, obj.rotation_euler = location, (0, 0, rotation_z)
    obj.empty_display_type, obj.empty_display_size = "SINGLE_ARROW", 1.0
    obj.bicycle.kind, obj.bicycle.spawn = "SPAWN", name
    return obj


def add_path_point(entity, location=None):
    pts = path_points(entity)
    basis = entity.matrix_basis.copy()  # == its world matrix (entities have no parent), and never stale
    last = basis @ pts[-1].location if pts else basis.translation
    obj = bpy.data.objects.new("%s path %02d" % (entity.name, len(pts) + 1), None)
    for c in entity.users_collection:
        c.objects.link(obj)
    obj.empty_display_type, obj.empty_display_size = "CUBE", 0.2
    obj.bicycle.kind = "PATH"
    obj.parent = entity  # its location is in the entity's space: it moves along with the entity
    world = Vector(location) if location is not None else last + Vector((0, 0, 1))
    obj.location = basis.inverted() @ world
    return obj


def path_points(entity):
    return sorted((c for c in entity.children if c.bicycle.kind == "PATH"), key=lambda c: c.name)


# ---- export -----------------------------------------------------------------------------------------------------------
def yup(v):
    return (v[0], v[2], -v[1])


def r3(v):
    return [round(float(x), 3) + 0.0 for x in v]


def write_bimg(image, path):
    w, h = image.size
    px = list(image.pixels)  # RGBA floats, bottom row first
    rows = []
    for y in range(h - 1, -1, -1):
        row = bytearray()
        for x in range(w):
            i = 4 * (y * w + x)
            r, g, b, a = (max(0, min(255, int(round(c * 255)))) for c in px[i:i + 4])
            row += struct.pack("<I", r | (g << 8) | (b << 16) | (a << 24))
        rows.append(bytes(row))
    with open(path, "wb") as f:
        f.write(b"BIMG" + struct.pack("<4H", w, h, 0, 0) + b"".join(rows))


def material_info(mat):
    """(diffuse rgba, image or None) from a material's Principled BSDF."""
    rgba = tuple(mat.diffuse_color) if mat else (0.8, 0.8, 0.8, 1.0)
    image = None
    if mat and mat.use_nodes:
        bsdf = next((n for n in mat.node_tree.nodes if n.type == "BSDF_PRINCIPLED"), None)
        if bsdf:
            base = bsdf.inputs["Base Color"]
            rgba = tuple(base.default_value)
            if base.is_linked and base.links[0].from_node.type == "TEX_IMAGE":
                image = base.links[0].from_node.image
                rgba = (1.0, 1.0, 1.0, rgba[3])
    return rgba, image


def _zup_to_yup():
    return Matrix(((1, 0, 0, 0), (0, 0, 1, 0), (0, -1, 0, 0), (0, 0, 0, 1)))


def rig_of(obj):
    m = next((m for m in obj.modifiers if m.type == "ARMATURE" and m.object), None)
    return m.object if m else None


def ordered_bones(arm):
    out = []

    def visit(b):
        out.append(b)
        for c in b.children:
            visit(c)
    for b in arm.data.bones:
        if b.parent is None:
            visit(b)
    return out


def export_skin(obj, rig, path, scene):
    C = _zup_to_yup()
    flat = lambda m: [m[r][c] for c in range(4) for r in range(4)]  # noqa: E731 (column major)
    bones = ordered_bones(rig)
    index = {b.name: i for i, b in enumerate(bones)}
    mesh_from_rig = obj.matrix_world.inverted() @ rig.matrix_world
    Ci = C.inverted()

    def to_mesh_yup(m):
        return C @ mesh_from_rig @ m @ Ci
    out = bytearray(b"BSKN")
    actions = [a for a in bpy.data.actions if any(fc.data_path.startswith("pose.bones") for fc in a.fcurves)]
    nverts = len(obj.data.vertices)
    out += struct.pack("<BBHHI", 1, 2, len(bones), len(actions), nverts)
    for b in bones:
        name = b.name.encode()
        out += struct.pack("<B", len(name)) + name
        out += struct.pack("<h", index[b.parent.name] if b.parent else -1)
        out += struct.pack("<16f", *flat(to_mesh_yup(b.matrix_local).inverted()))
    groups = {g.index: g.name for g in obj.vertex_groups}
    for v in obj.data.vertices:
        inf = sorted(((g.weight, index[groups[g.group]]) for g in v.groups
                      if groups.get(g.group) in index and g.weight > 0), reverse=True)[:4]
        if not inf:
            inf = [(1.0, 0)]
        total = sum(w for w, _ in inf)
        q = [int(round(255 * w / total)) for w, _ in inf]
        q[0] += 255 - sum(q)
        joints = [j for _, j in inf] + [0] * (4 - len(inf))
        q += [0] * (4 - len(q))
        out += struct.pack("<4B4B", *joints, *q)
    saved = rig.animation_data.action if rig.animation_data else None
    rig.data.pose_position = "POSE"
    for act in actions:
        rig.animation_data.action = act
        start, end = (int(x) for x in act.frame_range)
        frames = int(act.get("bicycle_frames", end - start))
        fps = scene.render.fps / scene.render.fps_base
        name = act.name.encode()
        out += struct.pack("<B", len(name)) + name + struct.pack("<fH", fps, frames)
        for f in range(frames):
            scene.frame_set(start + f)
            glob = [to_mesh_yup(rig.pose.bones[b.name].matrix) for b in bones]
            for i, b in enumerate(bones):
                local = glob[index[b.parent.name]].inverted() @ glob[i] if b.parent else glob[i]
                t, qq, _ = local.decompose()
                out += struct.pack("<3f4f", t.x, t.y, t.z, qq.x, qq.y, qq.z, qq.w)
    if saved:
        rig.animation_data.action = saved
    rig.data.pose_position = "REST"
    with open(path, "wb") as f:
        f.write(bytes(out))
    print("bicycle export: %s: %d joints, %d actions (%s), %d skinned verts" % (
        os.path.basename(path), len(bones), len(actions), ", ".join(a.name for a in actions), nverts))


def export_mesh(obj, out_dir, scene):
    """<out_dir>/<object>.obj + .mtl (+ textures as .bimg, + .bskin for rigged meshes). Static objects get their
    world transform baked in; "character" objects (genome models) stay in their own space."""
    role = obj.get("bicycle_role", "static")
    rig = rig_of(obj)
    layer = scene.view_layers[0]
    if rig:
        rig.data.pose_position = "REST"
    layer.update()
    ev = obj.evaluated_get(layer.depsgraph)
    mesh = ev.to_mesh()
    bm = bmesh.new()
    bm.from_mesh(mesh)
    bmesh.ops.triangulate(bm, faces=bm.faces[:])
    if role != "character":
        bm.transform(obj.matrix_world)
    bm.to_mesh(mesh)
    bm.free()
    mesh.calc_loop_triangles()
    uv_layer = mesh.uv_layers.active
    normals = mesh.corner_normals
    name = obj.name
    lines = ["# bicycle export of %s" % name, "mtllib %s.mtl" % name]
    for v in mesh.vertices:
        lines.append("v %.6f %.6f %.6f" % yup(v.co))
    for loop in mesh.loops:
        if uv_layer:
            u, v = uv_layer.data[loop.index].uv
            lines.append("vt %.6f %.6f" % (u, v))
    for loop in mesh.loops:
        lines.append("vn %.6f %.6f %.6f" % yup(normals[loop.index].vector))
    by_mat = {}
    for poly in mesh.polygons:
        by_mat.setdefault(poly.material_index, []).append(poly)
    mtl, textures = [], 0
    for mi, polys in sorted(by_mat.items()):
        mat = obj.material_slots[mi].material if mi < len(obj.material_slots) else None
        mname = mat.name if mat else "default"
        rgba, image = material_info(mat)
        mtl += ["newmtl %s" % mname, "Kd %.4f %.4f %.4f" % rgba[:3], "d %.4f" % rgba[3]]
        if image:
            write_bimg(image, os.path.join(out_dir, image.name + ".bimg"))
            mtl.append("map_Kd %s.png" % image.name)  # bicycle-pack uses the .bimg next to it
            textures += 1
        mtl.append("")
        lines.append("usemtl %s" % mname)
        for poly in polys:
            corners = []
            for li in poly.loop_indices:
                vi = mesh.loops[li].vertex_index + 1
                corners.append("%d/%s/%d" % (vi, (li + 1) if uv_layer else "", li + 1))
            lines.append("f " + " ".join(corners))
    with open(os.path.join(out_dir, name + ".obj"), "w") as f:
        f.write("\n".join(lines) + "\n")
    with open(os.path.join(out_dir, name + ".mtl"), "w") as f:
        f.write("\n".join(mtl))
    print("bicycle export: %s (%s): %d verts, %d tris, %d materials, %d textures" % (
        name, role, len(mesh.vertices), len(mesh.polygons), len(by_mat), textures))
    ev.to_mesh_clear()
    if rig:
        export_skin(obj, rig, os.path.join(out_dir, name + ".bskin"), scene)
    return role


def _set_dotted(t, dotted, v):
    parts = dotted.split(".")
    for p in parts[:-1]:
        t = t.setdefault(p, {})
    t[parts[-1]] = v


def _decode(p):
    t, v = p.ptype, p.value
    if t in ("number", "int"):
        try:
            f = float(v)
        except ValueError:
            return None
        return int(f) if t == "int" or f == int(f) else f
    if t == "bool":
        return v == "true"
    return v if v != "" else None


def world_of(obj):
    """obj's world matrix from its own location/rotation/scale and its parents' (matrix_world can be stale when a
    script just moved things)."""
    m = obj.matrix_basis
    if obj.parent:
        m = world_of(obj.parent) @ obj.matrix_parent_inverse @ m
    return m


def entity_entry(obj):
    b = obj.bicycle
    m = world_of(obj)
    entry = {"genome": b.genome, "key": obj.name, "pos": r3(yup(m.translation))}
    yaw = round(math.degrees(m.to_euler().z), 2) + 0.0
    if yaw:
        entry["yaw"] = yaw
    if b.persist:
        entry["persist"] = True
    for p in b.params:
        if p.ptype == "path":
            pts = path_offsets(obj)
            if pts:
                _set_dotted(entry, p.name, pts)
            continue
        chosen = p.ptype in ("scene", "genome") or p.ptype.startswith("spawn") or p.ptype.startswith("enum:")
        if not chosen and p.value == p.default and p.default != "":
            continue  # the genome's own default number/flag/text: not repeated per instance
        v = _decode(p)
        if v is not None:
            _set_dotted(entry, p.name, v)
    return entry


def path_offsets(obj):
    """The path from the entity's children: path points (by name) or a curve's points, relative to the entity."""
    origin = world_of(obj).translation
    world = [world_of(c).translation.copy() for c in path_points(obj)]
    if not world:
        curve = next((c for c in obj.children if c.type == "CURVE"), None)
        if curve:
            for sp in curve.data.splines:
                pts = sp.bezier_points if sp.type == "BEZIER" else sp.points
                for pt in pts:
                    world.append(world_of(curve) @ Vector(pt.co[:3]))
    if not world:
        return []
    return [[0, 0, 0]] + [r3(yup(w - origin)) for w in world]


def export_scene(scene, gdir=None, meshes_done=None):
    """Writes genome/level/<level>.lua (meshes, entities, spawns) and the meshes of one Blender scene."""
    gdir = gdir or genome_dir(scene)
    if not gdir:
        raise RuntimeError("bicycle: no genome folder (save the .blend in <game>/blender/, or set it in the Bicycle panel)")
    model_dir, body_dir, level_dir = (os.path.join(gdir, d) for d in ("model", os.path.join("body", "model"), "level"))
    for d in (model_dir, body_dir, level_dir):
        os.makedirs(d, exist_ok=True)
    meshes_done = meshes_done if meshes_done is not None else set()
    scene.view_layers[0].update()  # current world matrices (scripts move things without a redraw)
    objects, entities, spawns = [], [], {}
    for o in sorted(scene.objects, key=lambda o: o.name):
        k = o.bicycle.kind
        if k == "ENTITY" and o.bicycle.genome:
            entities.append(entity_entry(o))
        elif k == "SPAWN" and o.bicycle.spawn:
            m = world_of(o)
            spawns[o.bicycle.spawn] = r3(yup(m.translation)) + [round(math.degrees(m.to_euler().z), 2) + 0.0]
        elif o.type == "MESH" and k == "NONE" and o.get("bicycle_role") != "preview":
            role = o.get("bicycle_role", "static")
            out = body_dir if role == "character" else model_dir
            if o.name not in meshes_done:
                export_mesh(o, out, scene)
                meshes_done.add(o.name)
            if role != "character":
                objects.append({"name": o.name, "mesh": "model/" + o.name, "role": role})
    # meshes in the order they were authored (stable diffs), entities and spawns by name
    order = {o.name: i for i, o in enumerate(scene.objects)}
    objects.sort(key=lambda o: order.get(o["name"], 0))
    if not scene.bicycle.export_level:
        print("bicycle export: scene %s: models only" % scene.name)
        return None
    level = level_name(scene)
    path = os.path.join(level_dir, level + ".lua")
    blend = os.path.basename(bpy.data.filepath) or "an unsaved .blend"
    with open(path, "w") as f:
        f.write("-- written by tools/blender (bicycle add-on) from %s, Blender scene \"%s\": don't edit, re-export.\n" % (blend, scene.name))
        f.write("-- objects: the level's meshes; entities: genomes placed in it (genome/<name>.lua) with per-instance\n")
        f.write("-- parameters; spawns: { x, y, z, yaw } where doors and Scene.go put the player.\nreturn {\n  objects = {\n")
        for o in objects:
            f.write("    %s,\n" % lua(o))
        f.write("  },\n  entities = {\n")
        for e in entities:
            f.write("    %s,\n" % lua(e))
        f.write("  },\n  spawns = %s,\n}\n" % lua(spawns))
    print("bicycle export: %s: %d meshes, %d entities, %d spawns" % (path, len(objects), len(entities), len(spawns)))
    return path


def export_all(gdir=None):
    done, paths = set(), []
    for s in bpy.data.scenes:
        p = export_scene(s, gdir, done)
        if p:
            paths.append(p)
    return paths


# ---- UI -----------------------------------------------------------------------------------------------------------
if bpy:
    class BICYCLE_OT_refresh(bpy.types.Operator):
        bl_idname = "bicycle.refresh"
        bl_label = "Refresh genomes"
        bl_description = "Re-read the game's genome folder (new genomes, scenes, spawn points)"

        def execute(self, context):
            cat = catalog(refresh=True)
            for o in bpy.data.objects:
                if o.bicycle.kind == "ENTITY" and o.bicycle.genome in cat.genomes:
                    set_genome(o, o.bicycle.genome)
            self.report({"INFO"}, "%d genomes, %d scenes in %s" % (len(cat.genomes), len(cat.scenes), cat.dir))
            return {"FINISHED"}

    class BICYCLE_OT_add_entity(bpy.types.Operator):
        bl_idname = "bicycle.add_entity"
        bl_label = "Add entity"
        bl_description = "Place the chosen genome at the 3D cursor"
        bl_options = {"REGISTER", "UNDO"}
        genome: StringProperty()

        def execute(self, context):
            g = self.genome or context.scene.bicycle.add_genome
            if g not in catalog().genomes:
                self.report({"ERROR"}, "Pick a genome first")
                return {"CANCELLED"}
            for o in context.selected_objects:
                o.select_set(False)
            obj = add_entity(context.scene, g, context.scene.cursor.location.copy())
            obj.select_set(True)
            context.view_layer.objects.active = obj
            return {"FINISHED"}

    class BICYCLE_OT_add_spawn(bpy.types.Operator):
        bl_idname = "bicycle.add_spawn"
        bl_label = "Add spawn point"
        bl_description = "A spawn point at the 3D cursor (the arrow's direction is where the player faces)"
        bl_options = {"REGISTER", "UNDO"}

        def execute(self, context):
            name = context.scene.bicycle.spawn_name.strip() or "default"
            obj = add_spawn(context.scene, name, context.scene.cursor.location.copy())
            for o in context.selected_objects:
                o.select_set(False)
            obj.select_set(True)
            context.view_layer.objects.active = obj
            return {"FINISHED"}

    class BICYCLE_OT_add_path_point(bpy.types.Operator):
        bl_idname = "bicycle.add_path_point"
        bl_label = "Add path point"
        bl_description = "A point of this entity's path (1 m above the last one; move it where it should go)"
        bl_options = {"REGISTER", "UNDO"}

        def execute(self, context):
            ent = context.active_object
            if ent and ent.bicycle.kind == "PATH":
                ent = ent.parent
            if not ent or ent.bicycle.kind != "ENTITY":
                self.report({"ERROR"}, "Select an entity first")
                return {"CANCELLED"}
            p = add_path_point(ent)
            for o in context.selected_objects:
                o.select_set(False)
            p.select_set(True)
            context.view_layer.objects.active = p
            return {"FINISHED"}

    class BICYCLE_OT_export(bpy.types.Operator):
        bl_idname = "bicycle.export"
        bl_label = "Export level"
        bl_description = "Write genome/level/<level>.lua (meshes, entities, spawn points) and the meshes"
        all_scenes: BoolProperty(name="All scenes", default=False)

        def execute(self, context):
            try:
                paths = export_all() if self.all_scenes else [p for p in [export_scene(context.scene)] if p]
            except Exception as e:  # shown to the user, not a traceback
                self.report({"ERROR"}, str(e))
                return {"CANCELLED"}
            self.report({"INFO"}, "Wrote " + ", ".join(os.path.basename(p) for p in paths))
            return {"FINISHED"}

    class BICYCLE_PT_panel(bpy.types.Panel):
        bl_label = "Bicycle"
        bl_space_type = "VIEW_3D"
        bl_region_type = "UI"
        bl_category = "Bicycle"

        def draw(self, context):
            lay, scene = self.layout, context.scene
            cat = catalog()
            box = lay.box()
            box.label(text="Game", icon="FILE_FOLDER")
            box.prop(scene.bicycle, "genome_dir")
            box.label(text=cat.dir or "(save the .blend in <game>/blender/)")
            box.operator("bicycle.refresh", icon="FILE_REFRESH")
            box = lay.box()
            box.label(text="Level", icon="SCENE_DATA")
            box.prop(scene.bicycle, "export_level")
            box.prop(scene.bicycle, "level", text="Level name")
            box.label(text="-> genome/level/%s.lua" % level_name(scene))
            row = box.row()
            row.operator("bicycle.export", text="Export level", icon="EXPORT").all_scenes = False
            row.operator("bicycle.export", text="Export all scenes").all_scenes = True
            box = lay.box()
            box.label(text="Place", icon="ADD")
            box.prop(scene.bicycle, "add_genome_choice", text="Genome")
            box.operator("bicycle.add_entity", icon="EMPTY_AXIS")
            row = box.row()
            row.prop(scene.bicycle, "spawn_name")
            row.operator("bicycle.add_spawn", text="Add spawn point", icon="EMPTY_SINGLE_ARROW")
            obj = context.active_object
            if not obj:
                return
            b = obj.bicycle
            box = lay.box()
            if b.kind == "ENTITY":
                box.label(text="Entity: " + obj.name, icon="OUTLINER_OB_EMPTY")
                box.prop(b, "genome_choice", text="Genome")
                if b.genome not in cat.genomes:
                    box.label(text="No genome '%s' in this game" % b.genome, icon="ERROR")
                box.prop(b, "persist")
                for p in b.params:
                    row = box.row()
                    label = p.name
                    if p.ptype == "path":
                        row.label(text="%s: %d points" % (label, len(path_points(obj))))
                        row.operator("bicycle.add_path_point", text="Add path point", icon="ADD")
                    elif p.ptype in ("scene", "genome") or p.ptype.startswith("spawn") or p.ptype.startswith("enum:"):
                        row.prop(p, "choice", text=label)
                    elif p.ptype in ("number", "int"):
                        row.prop(p, "number", text=label)
                    elif p.ptype == "bool":
                        row.prop(p, "flag", text=label)
                    else:
                        row.prop(p, "value", text=label)
                if not b.params:
                    box.label(text="(this genome has no per-instance parameters)")
            elif b.kind == "SPAWN":
                box.label(text="Spawn point", icon="EMPTY_SINGLE_ARROW")
                box.prop(b, "spawn")
            elif b.kind == "PATH":
                box.label(text="Path point of " + (obj.parent.name if obj.parent else "?"), icon="CURVE_PATH")
                box.operator("bicycle.add_path_point", text="Add another point")
            else:
                box.label(text="%s: level geometry (role %s)" % (obj.name, obj.get("bicycle_role", "static")))

    CLASSES = (BicycleParam, BicycleObject, BicycleScene, BICYCLE_OT_refresh, BICYCLE_OT_add_entity,
               BICYCLE_OT_add_spawn, BICYCLE_OT_add_path_point, BICYCLE_OT_export, BICYCLE_PT_panel)


def register():
    for c in CLASSES:
        bpy.utils.register_class(c)
    bpy.types.Object.bicycle = PointerProperty(type=BicycleObject)
    bpy.types.Scene.bicycle = PointerProperty(type=BicycleScene)


def unregister():
    del bpy.types.Scene.bicycle
    del bpy.types.Object.bicycle
    for c in reversed(CLASSES):
        bpy.utils.unregister_class(c)


if __name__ == "__main__" and bpy:  # run from Blender's Text Editor: register for this session
    register()
