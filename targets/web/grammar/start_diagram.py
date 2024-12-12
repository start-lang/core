import re
import sys
from railroad import *
from io import StringIO

style = '''
svg.railroad-diagram {
    background-color: transparent;
}
svg.railroad-diagram path {
    stroke-width: 1;
    stroke: black;
    fill: rgba(0,0,0,0);
}
svg.railroad-diagram text {
    fill: white;
    font: 14px monospace;
    text-anchor: middle;
    white-space: pre;
}
svg.railroad-diagram text.diagram-text {
    font-size: 12px;
}
svg.railroad-diagram text.diagram-arrow {
    font-size: 16px;
}
svg.railroad-diagram text.label {
    text-anchor: start;
}
svg.railroad-diagram text.comment {
    fill: #5991ff;
    font: italic 12px monospace;
}
svg.railroad-diagram g.non-terminal text {
    /*font-style: italic;*/
}
svg.railroad-diagram rect {
    stroke-width: 1;
    stroke: black;
    fill: black;
}
svg.railroad-diagram rect.group-box {
    stroke: #5991ff;
    stroke-dasharray: 10 5;
    fill: none;
}
svg.railroad-diagram path.diagram-text {
    stroke-width: 3;
    stroke: black;
    fill: white;
    cursor: help;
}
svg.railroad-diagram g.diagram-text:hover path.diagram-text {
    fill: #eee;
}
'''
blog = '''
    svg.railroad-diagram {
        background-color: transparent;
    }
    svg.railroad-diagram path {
        stroke-width: 1;
        stroke: yellow;
        fill: transparent;
    }
    svg.railroad-diagram text {
        fill: white;
        font: 14px monospace;
        text-anchor: middle;
        white-space: pre;
    }
    svg.railroad-diagram text.diagram-text {
        font-size: 12px;
    }
    svg.railroad-diagram text.diagram-arrow {
        font-size: 16px;
    }
    svg.railroad-diagram text.label {
        text-anchor: start;
    }
    svg.railroad-diagram text.comment {
        font: italic 12px monospace;
    }
    svg.railroad-diagram g.non-terminal text {
        /*font-style: italic;*/
    }
    svg.railroad-diagram rect {
        stroke-width: 1;
        stroke: yellow;
        fill: transparent;
    }
    svg.railroad-diagram rect.group-box {
        stroke: rgb(251, 68, 98);
        stroke-dasharray: 10 5;
        fill: none;
    }
    svg.railroad-diagram path.diagram-text {
        stroke-width: 3;
        stroke: black;
        fill: white;
        cursor: help;
    }
    svg.railroad-diagram g.diagram-text:hover path.diagram-text {
        fill: #eee;
    }
'''

col1 = [
    HorizontalChoice(
        Group("\" \"", "nop"),
        Group("0-9+", "number"),
        Sequence(Group(Sequence("A-Z+_"), "id"), Optional(Group("^", "var"))),
    ),
    Group(Sequence("\"", ZeroOrMore(HorizontalChoice("\\\\", "\\\"", "any")), "\""), "string"),
    Group(ZeroOrMore(NonTerminal("statement")), "block"),
    Group(
        Sequence(
            "(", NonTerminal("block"),
            Optional(Sequence(":", NonTerminal("block"))), ")"
        ), "if"
    ),
    Group(Sequence("[", NonTerminal("block"), "]"), "while"),
    Group(
        Sequence(
            NonTerminal("id"), "{", NonTerminal("block"), "}"
        ), "function"
    ),
    HorizontalChoice(
        Group("r", "return"),
        Group(HorizontalChoice("c", "x"), "loop_op"),
    ),
]

col2 = [
    Group(HorizontalChoice("b", "s", "i", "f"), "type"),
    Group(Choice(0, HorizontalChoice("<", ">", "!", ";"), HorizontalChoice("@", "z", "#", "m")), "mem"),
    Group(Sequence("?", Choice(0, HorizontalChoice(">", "<", "=", "!", "h"), HorizontalChoice("l", "g", "?", "z"))), "cmp"),
    Group(HorizontalChoice("+", "-", "*", "/", "%"), "math"),
    Group(HorizontalChoice("$", "p", "o", "h"), "stack"),
    Group(HorizontalChoice("&", "|", "^", "<", ">", "~"), "bitwise"),
]

desktop = Diagram(
    Choice(0,
        Group(
            HorizontalChoice(
                Choice(2,
                    *col1
                ),
                Choice(2,
                    *col2
                ),
            ), "statement"
        ),
        Optional(NonTerminal("block"), "skip")
    )
)

mobile = Diagram(
    Choice(0,
        Group(
            Choice(2,
                *(col1 + col2)
            ), "statement"
        ),
        Optional(NonTerminal("block"), "skip")
    )
)

def gen(d, css=None):
    svg_buffer = StringIO()
    d.writeSvg(svg_buffer.write)
    svg_str = svg_buffer.getvalue()
    if css:
        svg_str = re.sub(
            r'<style>.*<\/style>',
            f'<style>{css}</style>',
            svg_str,
            flags=re.DOTALL
        )
    print(svg_str)

if len(sys.argv) > 2 and sys.argv[2] == 'blog':
    style = blog
if len(sys.argv) == 1 or sys.argv[1] == 'desktop':
    gen(desktop, style)
else:
    gen(mobile, style)
