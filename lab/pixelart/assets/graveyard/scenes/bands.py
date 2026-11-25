"""The graveyard's two grave bands: monuments laid out on solid ground only.

Ground spans without footing: upper band sinkhole x 4118-4306; lower band pits x 2390-2506
and 3134-3298; dstar's bush stands at 1104-1200.
Runs inside Blender after blender_kit/core.py and blender_kit/graves.py.
"""

UPPER_CAM = dict(x0=3744, x1=5280, y0=1368, y1=1560)   # 64 x 8 tiles
LOWER_CAM = dict(x0=816, x1=3528, y0=1536, y1=1728)    # 113 x 8 tiles

UPPER = [
    ("u_fleury_a", "dark", fleury_cross, 3796, (52,), dict(lean=-13, turn=18, seed=11)),
    ("u_stone_a", "stone", headstone2, 3846, (24, 34), dict(style="arch", emblem="wreath", lines=3, lean=4, seed=12)),
    ("u_pillar", "stone", pillar_urn, 3912, (66,), dict(seed=13, lean=2)),
    ("u_stone_b", "dark", headstone2, 3974, (26, 30), dict(style="shoulder", emblem="cross", lines=2, lean=-3, seed=14)),
    ("u_chest", "stone", chest_tomb, 4050, (56, 16), dict(seed=15, turn=14)),
    # sinkhole 4118-4306: nothing stands there
    ("u_celtic_a", "stone", celtic_cross2, 4338, (72,), dict(seed=16, lean=3)),
    ("u_stone_c", "stone", headstone2, 4400, (22, 40), dict(style="gothic", emblem="cross", lines=3, lean=-6, seed=17)),
    ("u_stone_d", "dark", headstone2, 4456, (24, 30), dict(style="arch", emblem="eye", lines=2, broken=True, seed=18, lean=5)),
    ("u_fence", None, None, 4500, (4590, 26), {}),
    ("u_stone_e", "stone", headstone2, 4546, (20, 26), dict(style="arch", emblem="cross", lines=2, seed=19, lean=-4, turn=26)),
    ("u_pillar_b", "dark", pillar_urn, 4644, (52,), dict(seed=20, broken=True)),
    ("u_stone_f", "stone", headstone2, 4712, (28, 34), dict(style="tablet", emblem="wreath", lines=4, seed=21, lean=-10)),
    ("u_stone_g", "dark", headstone2, 4790, (22, 26), dict(style="arch", emblem="none", lines=3, seed=22, lean=11, broken=True)),
    ("u_stone_h", "stone", headstone2, 4852, (26, 44), dict(style="arch", emblem="eye", lines=4, seed=23, lean=2)),
    ("u_celtic_b", "stone", celtic_cross2, 4940, (78,), dict(seed=24, lean=-5)),
    ("u_chest_b", "dark", chest_tomb, 5030, (48, 14), dict(seed=25, turn=18)),
    ("u_stone_i", "stone", headstone2, 5106, (22, 38), dict(style="gothic", emblem="cross", lines=3, seed=26, lean=-12)),
    ("u_fence_b", None, None, 5150, (5214, 22), {}),
    ("u_fleury_b", "dark", fleury_cross, 5250, (44,), dict(seed=27, lean=9)),
]

LOWER = [
    ("l_fleury_a", "dark", fleury_cross, 884, (50,), dict(lean=-15, seed=31)),
    ("l_stone_a", "stone", headstone2, 940, (24, 36), dict(style="arch", emblem="cross", lines=3, lean=5, seed=32)),
    ("l_chest", "dark", chest_tomb, 1036, (54, 15), dict(seed=33, turn=16)),
    # dstar's bush 1104-1200
    ("l_celtic_a", "stone", celtic_cross2, 1262, (74,), dict(seed=34, lean=3)),
    ("l_stone_b", "stone", headstone2, 1330, (22, 42), dict(style="gothic", emblem="cross", lines=3, lean=-6, seed=35)),
    ("l_fence", None, None, 1376, (1468, 26), {}),
    ("l_stone_c", "dark", headstone2, 1422, (20, 24), dict(style="arch", emblem="none", lines=2, broken=True, seed=36, lean=8)),
    ("l_pillar", "stone", pillar_urn, 1540, (68,), dict(seed=37)),
    ("l_stone_d", "stone", headstone2, 1612, (28, 32), dict(style="shoulder", emblem="wreath", lines=3, seed=38)),
    ("l_chest_b", "stone", chest_tomb, 1702, (52, 16), dict(seed=39, turn=12)),
    ("l_stone_e", "stone", headstone2, 1782, (24, 44), dict(style="arch", emblem="eye", lines=4, lean=-12, seed=40)),
    ("l_pillar_b", "dark", pillar_urn, 1862, (50,), dict(seed=41, broken=True)),
    ("l_stone_f", "stone", headstone2, 1930, (20, 26), dict(style="tablet", emblem="cross", lines=2, lean=4, seed=42)),
    ("l_fleury_b", "dark", fleury_cross, 2002, (56,), dict(lean=11, seed=43)),
    ("l_stone_g", "stone", headstone2, 2090, (28, 38), dict(style="gothic", emblem="wreath", lines=3, lean=-2, seed=44)),
    ("l_celtic_b", "stone", celtic_cross2, 2186, (68,), dict(seed=45, lean=-7)),
    ("l_stone_h", "dark", headstone2, 2256, (20, 22), dict(style="arch", emblem="none", lines=2, broken=True, seed=46)),
    ("l_fence_b", None, None, 2300, (2372, 24), {}),
    ("l_stone_i", "stone", headstone2, 2336, (20, 30), dict(style="arch", emblem="cross", lines=3, lean=-3, seed=47)),
    # pit 2390-2506
    ("l_stone_j", "stone", headstone2, 2536, (24, 40), dict(style="shoulder", emblem="eye", lines=3, lean=6, seed=48)),
    ("l_chest_c", "dark", chest_tomb, 2622, (50, 15), dict(seed=49, turn=20)),
    ("l_pillar_c", "dark", pillar_urn, 2716, (60,), dict(seed=50, lean=-3)),
    ("l_stone_k", "stone", headstone2, 2800, (22, 34), dict(style="arch", emblem="wreath", lines=3, lean=9, seed=51)),
    ("l_fleury_c", "dark", fleury_cross, 2872, (46,), dict(lean=-20, seed=52)),
    ("l_stone_l", "stone", headstone2, 2950, (28, 40), dict(style="gothic", emblem="cross", lines=4, seed=53)),
    ("l_celtic_c", "stone", celtic_cross2, 3050, (74,), dict(seed=54, lean=5)),
    ("l_column", "stone", pillar_urn, 3108, (34,), dict(seed=55, broken=True)),
    # pit 3134-3298
    ("l_stone_m", "stone", headstone2, 3316, (24, 30), dict(style="arch", emblem="cross", lines=3, lean=-4, seed=56)),
    ("l_fence_c", None, None, 3352, (3430, 22), {}),
    ("l_stone_n", "dark", headstone2, 3482, (26, 28), dict(style="shoulder", emblem="none", lines=3, lean=7, seed=57, broken=True)),
]



def build_upper(seed=21):
    return build_layout(UPPER, UPPER_CAM, 1556, [(4118, 4306)], seed)


def build_lower(seed=33):
    return build_layout(LOWER, LOWER_CAM, 1720, [(1104, 1200), (2390, 2506), (3134, 3298)], seed)
