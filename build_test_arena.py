"""
CursedCrypt - AI Barricade Detection Test Arena Builder
=========================================================
Bu script, BTS_CheckPathToTarget / AI-Barricade sistemini test etmek icin
iki ayri test bolgesi kurar:

  ZONE 1 - "Full Block" (cikmaz sokak):
    Duz bir koridor, ortasinda barikat, alternatif yol YOK.
    Amac: FindPathToActorSynchronously().IsPartial() == true durumunu
    tetiklemek -> AI barikati kirmali (Break Barricade branch).

  ZONE 2 - "Alternate Path" (halka / ring):
    Kare seklinde bir halka koridor. Bir kenarinda barikat var ama
    NavMesh uzerinden etrafindan dolasilabilir bir yol mevcut.
    Amac: IsPartial() == false durumunu tetiklemek -> AI barikati
    KIRMAMALI, etrafindan dolasmali (Option C mantigi).

Kullanilan olcu degerleri (Andac'in verdigi Approx Size degerlerinden
hesaplandi):
  - Enemy capsule:  radius 85, half-height 88
  - Player capsule: radius 85, half-height 106
  - Barikat:        199 x 199 x 77 (X x Y x Z)

CALISTIRMA:
  1) Edit > Plugins > "Python Editor Script Plugin" etkin oldugundan emin ol,
     degilse etkinlestirip editoru yeniden baslat.
  2) Bos/yeni bir level ac (onerilen: Maps/TestArena_AIBarricade).
  3) Output Log konsoluna (alt kisimdaki "Cmd" kutusu) sunu yaz:
         py "TAM_YOL/build_test_arena.py"
     ya da Window > Developer Tools > Python Console'dan calistir.
  4) Script bitince Content Browser'dan Barikat sinifinin dogru yola
     isaret ettigini (BARRICADE_CLASS_PATH) kontrol et; enemy BP'sini
     script'in biraktigi "Marker_" isimli TargetPoint'lerin yerine SEN
     surukle-birak ile koy (enemy BP'nin tam class path'ini bilmedigim
     icin script otomatik spawn etmiyor, sadece isaret birakiyor).

AYARLANABILIR DEGERLER asagida CONFIG bolumunde.
"""

import unreal

# ----------------------------------------------------------------------
# CONFIG - gerekirse buradan degistir
# ----------------------------------------------------------------------

CUBE_MESH_PATH = "/Engine/BasicShapes/Cube.Cube"
# Engine'in temel Cube mesh'i varsayilan olarak 100x100x100uu boyutundadir.
CUBE_DEFAULT_SIZE = 100.0

# BT_Enemy'deki SphereOverlapActors filtresinden dogrulanan gercek yol:
BARRICADE_CLASS_PATH = "/Game/CursedCrypt/BP_BarricadeBase.BP_BarricadeBase_C"

WALL_HEIGHT = 300.0
WALL_THICKNESS = 20.0
FLOOR_THICKNESS = 20.0

# ----------------------------------------------------------------------
# SETUP
# ----------------------------------------------------------------------

actor_subsystem = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
cube_mesh = unreal.EditorAssetLibrary.load_asset(CUBE_MESH_PATH)

spawned_actors = []


def spawn_box(location, size, label, actor_of=None):
    """Bir StaticMeshActor spawn eder, Cube mesh'i verilen boyuta olceklendirir.
    location: unreal.Vector - kutunun MERKEZ noktasi
    size: (x, y, z) tuple - kutunun toplam boyutu (uu)
    """
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.StaticMeshActor, location, unreal.Rotator(0, 0, 0)
    )
    mesh_comp = actor.static_mesh_component
    mesh_comp.set_static_mesh(cube_mesh)
    mesh_comp.set_mobility(unreal.ComponentMobility.STATIC)

    scale = unreal.Vector(
        size[0] / CUBE_DEFAULT_SIZE,
        size[1] / CUBE_DEFAULT_SIZE,
        size[2] / CUBE_DEFAULT_SIZE,
    )
    actor.set_actor_scale3d(scale)
    actor.set_actor_label(label, mark_dirty=True)
    spawned_actors.append(actor)
    return actor


def spawn_marker(location, label):
    """Enemy/Player'in elle yerlestirilecegi noktalari isaretlemek icin TargetPoint."""
    actor = actor_subsystem.spawn_actor_from_class(
        unreal.TargetPoint, location, unreal.Rotator(0, 0, 0)
    )
    actor.set_actor_label(label, mark_dirty=True)
    spawned_actors.append(actor)
    return actor


print("=== CursedCrypt Test Arena kurulumu basliyor ===")

# ----------------------------------------------------------------------
# ZONE 1 - FULL BLOCK (cikmaz koridor, alternatif yol YOK)
# X: 0 -> 800   (uzunluk 800)
# Y: -1350 -> -1100  (genislik 250)
# Barikat X=400'de tam genisligi kapatiyor.
# Giris (X=0) acik, dip (X=800) kapali (dead end).
# ----------------------------------------------------------------------

Z1_X0, Z1_X1 = 0.0, 800.0
Z1_Y0, Z1_Y1 = -1350.0, -1100.0
Z1_WIDTH = Z1_Y1 - Z1_Y0          # 250
Z1_LENGTH = Z1_X1 - Z1_X0         # 800
Z1_CY = (Z1_Y0 + Z1_Y1) / 2.0

print("-- Zone 1 (Full Block) --")

# Zemin
spawn_box(
    unreal.Vector((Z1_X0 + Z1_X1) / 2.0, Z1_CY, -FLOOR_THICKNESS / 2.0),
    (Z1_LENGTH, Z1_WIDTH, FLOOR_THICKNESS),
    "Z1_Floor",
)

# Yan duvarlar (uzun kenarlar)
spawn_box(
    unreal.Vector((Z1_X0 + Z1_X1) / 2.0, Z1_Y0, WALL_HEIGHT / 2.0),
    (Z1_LENGTH, WALL_THICKNESS, WALL_HEIGHT),
    "Z1_Wall_South",
)
spawn_box(
    unreal.Vector((Z1_X0 + Z1_X1) / 2.0, Z1_Y1, WALL_HEIGHT / 2.0),
    (Z1_LENGTH, WALL_THICKNESS, WALL_HEIGHT),
    "Z1_Wall_North",
)

# Dip duvari (dead end) - X1 ucunda
spawn_box(
    unreal.Vector(Z1_X1, Z1_CY, WALL_HEIGHT / 2.0),
    (WALL_THICKNESS, Z1_WIDTH, WALL_HEIGHT),
    "Z1_Wall_DeadEnd",
)
# X0 ucu ACIK birakiliyor (giris)

# Barikat - koridorun ortasinda (X=400), tam genisligi kapatiyor
barricade_class = unreal.EditorAssetLibrary.load_blueprint_class(BARRICADE_CLASS_PATH)
if barricade_class:
    barricade_loc = unreal.Vector(400.0, Z1_CY, 0.0)
    barricade = actor_subsystem.spawn_actor_from_class(
        barricade_class, barricade_loc, unreal.Rotator(0, 0, 0)
    )
    barricade.set_actor_label("Z1_TestBarricade", mark_dirty=True)
    spawned_actors.append(barricade)
    print("  Barikat spawn edildi: Z1_TestBarricade @ (400, %.0f, 0)" % Z1_CY)
else:
    print("  UYARI: BP_BarricadeBase yuklenemedi (%s) - barikati elle yerlestir!"
          % BARRICADE_CLASS_PATH)

# PlayerStart - barikatin OTESINDE (dip tarafinda)
player_start = actor_subsystem.spawn_actor_from_class(
    unreal.PlayerStart, unreal.Vector(750.0, Z1_CY, 100.0), unreal.Rotator(0, 0, 0)
)
player_start.set_actor_label("Z1_PlayerStart_BehindBarricade", mark_dirty=True)
spawned_actors.append(player_start)

# Enemy marker - giris tarafinda
spawn_marker(
    unreal.Vector(50.0, Z1_CY, 100.0), "Marker_Z1_EnemySpawnHere_DragEnemyBPHere"
)

print("  Zone 1 tamam: Enemy marker (50, %.0f) -> Player (750, %.0f), barikat X=400'de tam blok."
      % (Z1_CY, Z1_CY))

# ----------------------------------------------------------------------
# ZONE 2 - ALTERNATE PATH (halka / ring)
# Disi 900x900, ici 250-650 arasi bos (400x400 delik) -> 250uu genislikte halka yol.
# Alt kenarda (bottom segment) barikat var ama AI etrafindan dolasabilir.
# ----------------------------------------------------------------------

print("-- Zone 2 (Alternate Path / Ring) --")

OUT_MIN, OUT_MAX = 0.0, 900.0
IN_MIN, IN_MAX = 250.0, 650.0

# Zemin segmentleri (halka seklinde 4 parca)
spawn_box(unreal.Vector(450, 125, -FLOOR_THICKNESS / 2.0), (900, 250, FLOOR_THICKNESS), "Z2_Floor_Bottom")
spawn_box(unreal.Vector(450, 775, -FLOOR_THICKNESS / 2.0), (900, 250, FLOOR_THICKNESS), "Z2_Floor_Top")
spawn_box(unreal.Vector(125, 450, -FLOOR_THICKNESS / 2.0), (250, 400, FLOOR_THICKNESS), "Z2_Floor_Left")
spawn_box(unreal.Vector(775, 450, -FLOOR_THICKNESS / 2.0), (250, 400, FLOOR_THICKNESS), "Z2_Floor_Right")

# Dis duvarlar (900x900 cevre)
spawn_box(unreal.Vector(450, OUT_MIN, WALL_HEIGHT / 2.0), (900, WALL_THICKNESS, WALL_HEIGHT), "Z2_Wall_Outer_South")
spawn_box(unreal.Vector(450, OUT_MAX, WALL_HEIGHT / 2.0), (900, WALL_THICKNESS, WALL_HEIGHT), "Z2_Wall_Outer_North")
spawn_box(unreal.Vector(OUT_MIN, 450, WALL_HEIGHT / 2.0), (WALL_THICKNESS, 900, WALL_HEIGHT), "Z2_Wall_Outer_West")
spawn_box(unreal.Vector(OUT_MAX, 450, WALL_HEIGHT / 2.0), (WALL_THICKNESS, 900, WALL_HEIGHT), "Z2_Wall_Outer_East")

# Ic duvarlar (250-650 arasi ic bosluk cevresi)
spawn_box(unreal.Vector(450, IN_MIN, WALL_HEIGHT / 2.0), (400, WALL_THICKNESS, WALL_HEIGHT), "Z2_Wall_Inner_South")
spawn_box(unreal.Vector(450, IN_MAX, WALL_HEIGHT / 2.0), (400, WALL_THICKNESS, WALL_HEIGHT), "Z2_Wall_Inner_North")
spawn_box(unreal.Vector(IN_MIN, 450, WALL_HEIGHT / 2.0), (WALL_THICKNESS, 400, WALL_HEIGHT), "Z2_Wall_Inner_West")
spawn_box(unreal.Vector(IN_MAX, 450, WALL_HEIGHT / 2.0), (WALL_THICKNESS, 400, WALL_HEIGHT), "Z2_Wall_Inner_East")

# Barikat - alt (bottom) segmentin ortasinda, koridoru kapatiyor ama halka acik
if barricade_class:
    barricade2 = actor_subsystem.spawn_actor_from_class(
        barricade_class, unreal.Vector(450.0, 125.0, 0.0), unreal.Rotator(0, 0, 0)
    )
    barricade2.set_actor_label("Z2_RingBarricade", mark_dirty=True)
    spawned_actors.append(barricade2)
    print("  Barikat spawn edildi: Z2_RingBarricade @ (450, 125, 0)")

# Enemy / Player marker'lari (halka testinde manuel tasima icin)
spawn_marker(unreal.Vector(775, 450, 100), "Marker_Z2_EnemySpawnHere_DragEnemyBPHere")
spawn_marker(unreal.Vector(450, 775, 100), "Marker_Z2_PlayerTargetHere")

print("  Zone 2 tamam: Enemy marker sag segmentte, hedef ust segmentte, barikat alt segmentte.")

# ----------------------------------------------------------------------
# NAVMESH BOUNDS VOLUME - her iki bolgeyi de kapsayacak sekilde
# ----------------------------------------------------------------------

print("-- NavMeshBoundsVolume --")

nav_min = unreal.Vector(-100.0, -1450.0, -50.0)
nav_max = unreal.Vector(950.0, 950.0, 400.0)
nav_size = nav_max - nav_min
nav_center = (nav_min + nav_max) / 2.0

nav_volume = actor_subsystem.spawn_actor_from_class(
    unreal.NavMeshBoundsVolume, nav_center, unreal.Rotator(0, 0, 0)
)
# NavMeshBoundsVolume default brush boyutu 200x200x200uu'dur.
nav_scale = unreal.Vector(nav_size.x / 200.0, nav_size.y / 200.0, nav_size.z / 200.0)
nav_volume.set_actor_scale3d(nav_scale)
nav_volume.set_actor_label("TestArena_NavMeshBounds", mark_dirty=True)
spawned_actors.append(nav_volume)

print("  NavMeshBoundsVolume spawn edildi, olceklendi -> her iki zone'u kapsiyor.")

# ----------------------------------------------------------------------
# BITIS
# ----------------------------------------------------------------------

print("=== Kurulum tamam: %d aktor spawn edildi ===" % len(spawned_actors))
print("Sonraki adimlar:")
print("  1) 'P' tusuyla NavMesh preview'ini kontrol et (yesil = walkable).")
print("  2) Enemy BP'ni Marker_Z1_EnemySpawnHere / Marker_Z2_EnemySpawnHere noktalarina surukle.")
print("  3) Zone 1'de: Player barikatin OTESINDE -> AI path bulamamali (partial) -> kirma denemeli.")
print("  4) Zone 2'de: Player ust segmentte -> AI halkadan dolasip ulasmali, barikati KIRMAMALI.")
