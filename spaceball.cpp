/*
 * $Id: spaceball.cpp 1336 2014-12-08 09:29:59Z justin $
 * Copyright (C) 2009 Lucid Fusion Labs

 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "core/app/gui.h"
#include "core/app/network.h"
#include "core/web/browser.h"
#include "core/game/game.h"

#include "spaceballserv.h"

namespace LFL {
DEFINE_bool  (draw_fps,      true,                                  "Draw FPS");
DEFINE_int   (default_port,  27640,                                 "Default port");
DEFINE_string(master,        "lucidfusionlabs.com:27994/spaceball", "Master server list");
DEFINE_string(player_name,   "",                                    "Player name");
DEFINE_bool  (first_run,     true,                                  "First run of program");

typedef Particles<256, 1, true> BallTrails;
typedef Particles<256, 1, true> ShootingStars;
typedef Particles<1024, 1, true> Fireworks;
typedef Particles<16, 1, true> Thrusters;
typedef SpaceballGame::Ship MyShip;
typedef SpaceballGame::Ball MyBall;

struct MyAppState {
  AssetMap asset;
  SoundAssetMap soundasset;
  TextureArray caust;
  Shader fadershader, warpshader, explodeshader;
} *my_app;

Geometry *FieldGeometry(const Color &rg, const Color &bg, const Color &fc) {
  vector<v3> verts, norm;
  vector<v2> tex;
  vector<Color> col;
  SpaceballGame::FieldDefinition *fd = SpaceballGame::FieldDefinition::get();
  v3 goals = SpaceballGame::Goals::get(), up=v3(0,1,0), fwd=v3(0,0,1), rev(0,0,-1);
  float tx=10, ty=10;
  int ci=0;

  /* field */
  PushBack(&verts, &norm, &tex, &col, fd->B, up, v2(0,  0),  fc.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->C, up, v2(tx, 0),  fc.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->G, up, v2(tx, ty), fc.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->B, up, v2(0,  0),  fc.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->G, up, v2(tx, ty), fc.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->F, up, v2(0,  ty), fc.a(10 + ci++ * 6.0/255));
  tx *= .15;
  ty *= .15;

  /* red goal */
  ci = 0;
  PushBack(&verts, &norm, &tex, &col, fd->B * goals, rev, v2(0,  0),  rg.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->C * goals, rev, v2(tx, 0),  rg.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->D * goals, rev, v2(tx, ty), rg.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->B * goals, rev, v2(0,  0),  rg.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->D * goals, rev, v2(tx, ty), rg.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->A * goals, rev, v2(0,  ty), rg.a(10 + ci++ * 6.0/255));

  /* blue goal */
  ci = 0;
  PushBack(&verts, &norm, &tex, &col, fd->F * goals, fwd, v2(0,  0),  bg.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->G * goals, fwd, v2(tx, 0),  bg.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->H * goals, fwd, v2(tx, ty), bg.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->F * goals, fwd, v2(0,  0),  bg.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->H * goals, fwd, v2(tx, ty), bg.a(10 + ci++ * 6.0/255));
  PushBack(&verts, &norm, &tex, &col, fd->E * goals, fwd, v2(0,  ty), bg.a(10 + ci++ * 6.0/255));

  return new Geometry(GraphicsDevice::Triangles, verts.size(), &verts[0], 0, &tex[0], &col[0]);
}

Geometry *FieldLines(float tx, float ty) {
  SpaceballGame::FieldDefinition *fd = SpaceballGame::FieldDefinition::get();
  vector<v3> verts; vector<v2> tex;
  PushBack(&verts, &tex, fd->B, v2(0,  0));
  PushBack(&verts, &tex, fd->C, v2(tx, 0));
  PushBack(&verts, &tex, fd->G, v2(tx, ty));
  PushBack(&verts, &tex, fd->B, v2(0,  0));
  PushBack(&verts, &tex, fd->G, v2(tx, ty));
  PushBack(&verts, &tex, fd->F, v2(0,  ty));
  return new Geometry(GraphicsDevice::Triangles, verts.size(), &verts[0], 0, &tex[0], NullPointer<Color>());
}

void ShipDraw(GraphicsDevice *gd, Asset *a, Entity *e) {
  static unique_ptr<Geometry> stripes = Geometry::LoadOBJ(unique_ptr<File>(Asset::OpenFile("ship_stripes.obj")).get());
  Scene::Select(gd, stripes.get());
  gd->SetColor(e->color1);
  Scene::Draw(gd, stripes.get(), e);

  Scene::Select(gd, a);
  Shader *anim_shader = 0;
  if (e->animation.ShaderActive()) {
    anim_shader = e->animation.shader;
    gd->UseShader(anim_shader);
    anim_shader->SetUniform1f("time", e->animation.Percent());
  }
  Scene::Draw(gd, a->geometry, e);
  if (anim_shader) gd->UseShader(0);

  static Timer lightning_timer;
  static float last_lightning_offset = 0;
  static int lightning_texcoord_min_int_x = 0;
  static Font *lightning_font = app->fonts->Get("lightning");
  static Glyph *lightning_glyph = lightning_font->FindGlyph(2);
  static unique_ptr<Geometry> lightning_obj;
  if (!lightning_obj) {
    float lightning_glyph_texcoord[4];
    memcpy(lightning_glyph_texcoord, lightning_glyph->tex.coord, sizeof(lightning_glyph_texcoord));
    lightning_glyph_texcoord[Texture::maxx_coord_ind] *= .1;
    lightning_obj = Geometry::LoadOBJ(unique_ptr<File>(Asset::OpenFile("ship_lightning.obj")).get(),
                                      lightning_glyph_texcoord);
  }
  gd->BindTexture(GraphicsDevice::Texture2D, lightning_glyph->tex.ID);

  float lightning_offset = (e->namehash % 11) / 10.0;
  lightning_obj->ScrollTexCoord(-.4 * ToFSeconds(lightning_timer.GetTime(true)).count(),
                                lightning_offset - last_lightning_offset,
                                &lightning_texcoord_min_int_x);
  last_lightning_offset = lightning_offset;

  Scene::Select(gd, lightning_obj.get());
  gd->EnableBlend();
  Color c = e->color1;
  c.scale(2.0);
  gd->SetColor(c);
  Scene::Draw(gd, lightning_obj.get(), e);
}

void SetInitialCameraPosition(Entity *cam) {
  // position camera for a nice earth shot; from command 'campos'
  cam->pos = v3(5.54,1.70,4.39);
  cam->ort = v3(-0.14,0.02,-0.69);
  cam->up = v3(0.01,1.00,0.02);
}

struct SpaceballClient : public GameClient {
  Entity *ball=0;
  int last_scored_team=0;
  string last_scored_PlayerName;
  vector<v3> thrusters_transform;
  function<void(const string&, const string&, int)> map_changed_cb;
  SpaceballClient(Game *w, GUI *PlayerList, TextArea *Chat) : GameClient(w, PlayerList, Chat) {
    thrusters_transform.push_back(v3(-.575, -.350, -.1));
    thrusters_transform.push_back(v3( .525, -.350, -.1));
    thrusters_transform.push_back(v3(-.025,  .525, -.1));
  }

  void MoveBoost(unsigned) { SpaceballGame::Ship::set_boost(&control); }
  void NewEntityCB(Entity *e) {
    Asset *a = e->asset;
    if (!a) return;
    if (a->name == string("ball")) ball = e;
    if (a->particleTexID) {
      Thrusters *thrusters = new Thrusters("Thrusters", true, .1, .25, 0, 0.1);
      thrusters->emitter_type = Thrusters::Emitter::GlowFade;
      thrusters->texture = a->particleTexID;
      thrusters->ticks_step = 10;
      thrusters->gravity = -0.1;
      thrusters->age_min = .2;
      thrusters->age_max = .6;
      thrusters->radius_decay = true;
      thrusters->billboard = true;
      thrusters->pos_transform = &thrusters_transform;
      thrusters->move_with_pos = true;
      e->particles = thrusters;
    }
  }

  void DelEntityCB(Entity *e) {
    if (!e) return;
    if (ball == e) ball = 0;
    if (e->particles) delete static_cast<Thrusters*>(e->particles);
  }

  void AnimationChange(Entity *e, int NewID, int NewSeq) {
    static SoundAsset *bounce = my_app->soundasset("bounce");
    if      (NewID == SpaceballGame::AnimShipBoost) { if (FLAGS_enable_audio) app->PlaySoundEffect(bounce, e->pos, e->vel); }
    else if (NewID == SpaceballGame::AnimExplode)   e->animation.Start(&my_app->explodeshader);
  }

  void RconRequestCB(const string &cmd, const string &arg, int seq) { 
    // INFO("cmd: ", cmd, " ", arg);
    if (cmd == "goal") {
      last_scored_team = atoi(arg); 
      const char *scoredby = strchr(arg.c_str(), ' ');
      last_scored_PlayerName = scoredby ? scoredby+1 : "";
      INFO(last_scored_PlayerName, " scores for ", last_scored_team == Game::Team::Red ? "red" : "blue");

      unsigned updateInterval = (last.time_recv_WorldUpdate[0] - last.time_recv_WorldUpdate[1]).count();
      replay.start_ind = max(1, int(last.WorldUpdate.size() - (SpaceballGame::ReplaySeconds-1) * 1000.0 / updateInterval));
      replay.while_seq = last.seq_WorldUpdate = seq;
      replay.start = Now();

      Entity *cam = &world->scene->cam;
      cam->pos = v3(1.73,   2.53, 16.83);
      cam->up  = v3(-0.01,  0.98, -0.19);
      cam->ort = v3(-0.03, -0.13, -0.69);

      if (last_scored_team == Game::Team::Blue) {
        cam->pos.z *= -1;
        cam->ort.z *= -1;
        cam->up.z *= -1;
      }
      cam->ort.Norm();
      cam->up.Norm();
    }
    else if (cmd == "win") {
      gameover.start_ind = atoi(arg);
      gameover.while_seq = last.seq_WorldUpdate = seq;
      gameover.start = Now();
    } else if (cmd == "map") {
      vector<string> args;
      Split(arg, isspace, &args);
      if (args.size() != 3) return ERROR("map ", arg);
      map_changed_cb(args[0], args[1], atoi(args[2]));
    } else {
      return ERROR("unknown rcon: ", cmd, " ", arg);
    }
  }
};

struct TeamSelectGUI : public GUI {
  vector<SpaceballTeam> *teams;
  FontRef font, bright_font, glow_font, team_font;
  Widget::Button start_button;
  vector<Widget::Button> team_buttons;
  int home_team=0, away_team=0;

  TeamSelectGUI(Window *W) : GUI(W), teams(SpaceballTeam::GetList()),
  font       (FontDesc(FLAGS_font,                 "", 8, Color::grey80)),
  bright_font(FontDesc(FLAGS_font,                 "", 8, Color::white)),
  glow_font  (FontDesc(StrCat(FLAGS_font, "Glow"), "", 8, Color::white)),
  team_font  (FontDesc("sbmaps")),
  start_button(this, 0, "start", MouseController::CB(bind(&TeamSelectGUI::Start, this))) {
    start_button.outline_topleft     = &Color::grey80;
    start_button.outline_bottomright = &Color::grey40;
    start_button.solid = &Color::grey60;
    team_buttons.resize(teams->size());
    home_team = Rand(size_t(0), team_buttons.size()-1);
  }

  void SetHomeTeamIndex(int n) { home_team = n; child_box.Clear(); }
  void Start() { ShellRun("local_server"); }

  void Draw(Shader *MyShader) {
    GraphicsContext gc(root->gd);
    glShadertoyShaderWindows(gc.gd, MyShader, Color(25, 60, 130, 120), box);
    GUI::Draw();
    gc.gd->SetColor(Color::white);
    gc.gd->SetColor(Color::grey20); BoxTopLeftOutline    ().Draw(&gc, team_buttons[home_team].GetHitBoxBox());
    gc.gd->SetColor(Color::grey60); BoxBottomRightOutline().Draw(&gc, team_buttons[home_team].GetHitBoxBox());
    gc.gd->SetColor(Color::grey80); BoxTopLeftOutline    ().Draw(&gc, box);
    gc.gd->SetColor(Color::grey40); BoxBottomRightOutline().Draw(&gc, box);
  }

  void Layout() {
    CHECK(font.Load() && bright_font.Load() && glow_font.Load() && team_font.Load());
    for (int i=0; i<team_buttons.size(); i++) {
      team_buttons[i] =
        Widget::Button(this, team_font->FindGlyph((*teams)[i].font_index), (*teams)[i].name,
                       MouseController::CB(bind(&TeamSelectGUI::SetHomeTeamIndex, this, i)));
      team_buttons[i].outline_w = 1;
      team_buttons[i].outline_topleft     = &Color::grey60;
      team_buttons[i].outline_bottomright = &Color::grey20;
    }

    box = root->Box(.1, .1, .8, .8);
    int bw=box.w*2/13.0, bh=bw, sx=bw/2, px=(box.w - (bw*4 + sx*3))/2;
    Flow flow(&box, font, ResetGUI());
    flow.AppendNewlines(1);
    flow.p.x += px;
    for (int i = 0; i < team_buttons.size(); i++) {
      team_buttons[i].v_align = VAlign::Bottom;
      team_buttons[i].box = Box(bw, bh);
      team_buttons[i].Layout(&flow, home_team == i ? glow_font : font);
      flow.p.x += sx;

      if ((i+1) % 4 != 0) continue;
      flow.AppendNewlines(2);
      if (i+1 < team_buttons.size()) flow.p.x += px;
    }
    flow.layout.align_center = 1;
    start_button.box = root->Box(.4, .05);
    start_button.Layout(&flow, bright_font);
    flow.Complete();
  }
};

struct MyGameWindow : public GUI {
  const int default_transport_protocol=Protocol::UDP;
#ifdef LFL_EMSCRIPTEN
  const int local_transport_protocol=Protocol::InProcess;
#else
  const int local_transport_protocol=Protocol::UDP;
#endif
#define LFL_BUILTIN_SERVER
#ifdef  LFL_BUILTIN_SERVER
  SpaceballServer *builtin_server=0;
  bool             builtin_server_enabled=0;
#endif
  SpaceballClient *server=0;
  SpaceballSettings sbsettings;
  Scene scene;
  Game *world=0;
  GameMenuGUI *menubar=0;
  GameChatGUI *chat=0;
  GamePlayerListGUI *playerlist=0;
  TeamSelectGUI *team_select=0;
  GameMultiTouchControls *touchcontrols=0;
  vector<string> save_settings = {"player_name", "first_run", "msens"};
  unsigned fb_tex1=0, fb_tex2=0;
  Time map_started = Now();
  int map_transition=0, map_transition_start=0, caust_ind=0;
  bool draw_skybox_only=0;
  FrameBuffer framebuffer;
  Color home_goal_color, away_goal_color;
  HelperGUI *helper=0;
  Entity *star_particles, *ball_particles;
  BallTrails ball_trail;
  ShootingStars shooting_stars;
  Fireworks fireworks;
  vector<v3> fireworks_positions;
  SpaceballTeam *home_team=0, *away_team=0;
  Skybox skybox;

  MyGameWindow(Window *W) : GUI(W), framebuffer(W->gd), ball_trail("BallTrails", true, .05, .05, 0, 0),
  shooting_stars("ShootingStars", true, .1, .2, 0, 0), fireworks("Fireworks", true)
  {
    // field
    scene.Add(new Entity("lines", my_app->asset("lines")));
    scene.Add(new Entity("field", my_app->asset("field"),
              Entity::DrawCB(bind(&TextureArray::DrawSequence, &my_app->caust, _1, _2, &caust_ind))));

    // ball trail
    ball_trail.emitter_type = BallTrails::Emitter::Sprinkler | BallTrails::Emitter::RainbowFade;
#if 0
    ball_trail.emitter_type = BallTrails::Emitter::Sprinkler | BallTrails::Emitter::GlowFade;
    ball_trail.floor = true;
    ball_trail.floorval = Spaceball::FieldDefinition::get()->B.y;
#endif
    ball_trail.ticks_step = 10;
    ball_trail.gravity = -3;
    ball_trail.age_min = 1;
    ball_trail.age_max = 3;
    ball_trail.radius_decay = false;
    ball_trail.texture = my_app->asset("particles")->tex.ID;
    ball_trail.billboard = true;
    ball_particles = new Entity("ball_particles", my_app->asset("particles"),
                                Entity::DrawCB(bind(&BallTrails::AssetDrawCB, &ball_trail, W->gd, _1, _2)));
    scene.Add(ball_particles);

    // shooting stars
    my_app->asset("stars")->tex.ID = my_app->asset("particles")->tex.ID;
    shooting_stars.texture = my_app->asset("particles")->tex.ID;
    shooting_stars.burst = 16;
    shooting_stars.color = Color(1.0, 1.0, 1.0, 1.0);
    shooting_stars.ticks_step = 10;
    shooting_stars.gravity = 0;
    shooting_stars.age_min = .2;
    shooting_stars.age_max = .6;
    shooting_stars.radius_decay = true;
    shooting_stars.billboard = true;
    shooting_stars.vel = v3(0, 0, -1);
    shooting_stars.always_on = false;
    star_particles = new Entity("star_particles", my_app->asset("stars"),
                                Entity::DrawCB(bind(&ShootingStars::AssetDrawCB, &shooting_stars, W->gd, _1, _2)));
    star_particles->pos = v3(0, -4, 5);
    // scene.add(star_particles);

    // fireworks
    fireworks_positions.resize(2);
    fireworks.texture = my_app->asset("particles")->tex.ID;
    fireworks.pos_transform = &fireworks_positions;
    fireworks.rand_color = true;

    menubar = W->AddGUI(make_unique<GameMenuGUI>(W, FLAGS_master.c_str(), FLAGS_default_port, &my_app->asset("title")->tex));
    menubar->EnableParticles(&scene.cam, &my_app->asset("glow")->tex);
    menubar->tab3_player_name.AssignInput(FLAGS_player_name);
    menubar->settings = &sbsettings;
    menubar->Activate();
    menubar->selected = 1;
    W->gui.push_back(&menubar->topbar);
    playerlist = W->AddGUI(make_unique<GamePlayerListGUI>(W, "Spaceball 6006", "Team 1: silver", "Team 2: ontario"));
    team_select = W->AddGUI(make_unique<TeamSelectGUI>(W));

    chat = new GameChatGUI(root, 't', reinterpret_cast<GameClient**>(&server));
    chat->Write("Enter to grab mouse");
    chat->write_last = Time(0);

    world = new SpaceballGame(&scene);
    server = new SpaceballClient(world, playerlist, chat);
    server->map_changed_cb = bind(&MyGameWindow::HandleMapChanged, this, _1, _2, _3);

    // init frame buffer
    framebuffer.Create(W->width, W->height, FrameBuffer::Flag::CreateTexture | FrameBuffer::Flag::ReleaseFB);
    fb_tex1 = framebuffer.tex.ID;
    framebuffer.AllocTexture(&fb_tex2);

    SpaceballTeam *home = SpaceballTeam::GetRandom();
    LoadMap(home->name, SpaceballTeam::GetRandom(home)->name);
    SetInitialCameraPosition(&scene.cam);

    if (FLAGS_multitouch) {
      touchcontrols = new GameMultiTouchControls(server);
      helper = new HelperGUI(root);
      point space(W->width*.02, W->height*.05);
      const Box &lw = touchcontrols->lpad_win, &rw = touchcontrols->rpad_win;
      helper->AddLabel(Box(lw.x + lw.w*.15, lw.y + lw.h*.5,  1, 1), "move left",     HelperGUI::Hint::UPLEFT,  space);
      helper->AddLabel(Box(lw.x + lw.w*.85, lw.y + lw.h*.5,  1, 1), "move right",    HelperGUI::Hint::UPRIGHT, space);
      helper->AddLabel(Box(lw.x + lw.w*.5,  lw.y + lw.h*.85, 1, 1), "move forward",  HelperGUI::Hint::UP,      space);
      helper->AddLabel(Box(lw.x + lw.w*.5,  lw.y + lw.h*.15, 1, 1), "move back",     HelperGUI::Hint::DOWN,    space);
      helper->AddLabel(Box(rw.x + rw.w*.15, rw.y + rw.h*.5,  1, 1), "turn left",     HelperGUI::Hint::UPLEFT,  space);
      helper->AddLabel(Box(rw.x + rw.w*.85, rw.y + rw.h*.5,  1, 1), "turn right",    HelperGUI::Hint::UPRIGHT, space);
      helper->AddLabel(Box(rw.x + rw.w*.5,  rw.y + rw.h*.85, 1, 1), "burst forward", HelperGUI::Hint::UP,      space);
      helper->AddLabel(Box(rw.x + rw.w*.5,  rw.y + rw.h*.15, 1, 1), "change player", HelperGUI::Hint::DOWN,    space);
      helper->AddLabel(Box(W->width*(W->multitouch_keyboard_x + .035), W->height*.025, 1, 1), "keyboard", HelperGUI::Hint::UPLEFT, space);
      helper->AddLabel(menubar->topbar.box, "options menu", HelperGUI::Hint::DOWN, point(space.x, W->height*.15));
    }
  }

  void DisableLocalServer() {
#ifdef LFL_BUILTIN_SERVER
    if (builtin_server_enabled) {
      builtin_server_enabled = false;
      app->net->Shutdown(builtin_server->svc);
      app->net->Disable(builtin_server->svc);
    }
#endif
  }

  void EnableLocalServer(int game_type) {
    DisableLocalServer();

#ifdef LFL_BUILTIN_SERVER
    if (!builtin_server) {
      builtin_server = new SpaceballServer(StrCat(FLAGS_player_name, "'s server"), 20, &my_app->asset.vec);
      builtin_server->bots = new SpaceballBots(builtin_server->world);
      builtin_server->World()->game_finished_cb = bind(&MyGameWindow::HandleGameFinished, this, builtin_server->World());
      builtin_server->InitTransport(local_transport_protocol, FLAGS_default_port);
      if (local_transport_protocol == Protocol::InProcess) server->inprocess_server = builtin_server->inprocess_transport;
    }

    if (!builtin_server_enabled) {
      builtin_server_enabled = true;
      app->net->Enable(builtin_server->svc);
    }

    SpaceballGame *world = builtin_server->World();
    if (menubar->selected == 1) world->game_players = SpaceballSettings::PLAYERS_SINGLE;
    else                        world->game_players = SpaceballSettings::PLAYERS_MULTIPLE;
    world->game_type = sbsettings.GetIndex(SpaceballSettings::GAME_TYPE);
    world->game_limit = sbsettings.GetIndex(SpaceballSettings::GAME_LIMIT);
    world->game_control = sbsettings.GetIndex(SpaceballSettings::GAME_CONTROL);
    bool tourny = world->game_type == SpaceballSettings::TYPE_TOURNAMENT;
    if (!tourny) world->RandomTeams();
    else { 
      world->home = &(*team_select->teams)[team_select->home_team];
      world->away = SpaceballTeam::GetRandom(world->home);
    }

    builtin_server->bots->Clear();
    bool empty = game_type == SpaceballSettings::TYPE_EMPTYCOURT, spectator = game_type == SpaceballSettings::TYPE_SIMULATION;
    if (!empty) {
      builtin_server->bots->Insert(SpaceballGame::PlayersPerTeam*2);
      if (!spectator) builtin_server->bots->RemoveFromTeam(tourny ? Game::Team::Home : Game::Team::Random());
    }

    world->Reset();
#endif    
  }

  void HandleGameFinished(SpaceballGame *world) {
    if (world->game_players == SpaceballSettings::PLAYERS_MULTIPLE) return world->StartNextGame(builtin_server);

    DisableLocalServer();
    server->Reset();

    if (world->game_type == SpaceballSettings::TYPE_TOURNAMENT) team_select->active = true;
    else { menubar->Activate(); menubar->selected = 1; }

    SetInitialCameraPosition(&scene.cam);
  }

  void HandleMapChanged(const string &home, const string &away, int t) {
    draw_skybox_only = true;
    framebuffer.Attach(fb_tex1);
    root->RenderToFrameBuffer(&framebuffer);
    LoadMap(home, away);
    framebuffer.Attach(fb_tex2);
    root->RenderToFrameBuffer(&framebuffer);
    draw_skybox_only = false;
    map_started = Now() - Time(t);
    map_transition = map_transition_start = Seconds(3).count();
  }
  
  void LoadMap(const string &home_name, const string &away_name) {
    home_team = SpaceballTeam::Get(home_name);
    away_team = SpaceballTeam::Get(away_name);
    if (!home_team || !away_team) { ERROR("unknown team: ", home_name, " or ", away_name); return; }
    skybox.Load(home_team->skybox_name);

    home_goal_color = home_team->goal_color;
    away_goal_color = away_team->goal_color;
    my_app->asset("shipred" )->col = home_team->ship_color.diffuse;
    my_app->asset("shipblue")->col = away_team->ship_color.diffuse;

    float hh, hs, hv, ah, as, av;
    home_goal_color.ToHSV(&hh, &hs, &hv);
    away_goal_color.ToHSV(&ah, &as, &av);
    float angle_dist = min(360 - fabs(hh - ah), fabs(hh - ah));
    if (angle_dist < 60) {
      bool reverse = ah < hh;
      away_goal_color = Color::FromHSV(ah + (reverse ? -180 : 180), as, av);
      away_team->ship_color.diffuse.ToHSV(&ah, &as, &av);
      my_app->asset("shipblue")->col = Color::FromHSV(ah + (reverse ? -180 : 180), as, av);
    }

    Asset *field = my_app->asset("field");
    delete field->geometry;
    field->geometry = FieldGeometry(home_goal_color, away_goal_color, home_team->field_color);

    root->gd->EnableLight(0);
    root->gd->Light(0, GraphicsDevice::Ambient,  home_team->light.color.ambient.x);
    root->gd->Light(0, GraphicsDevice::Diffuse,  home_team->light.color.diffuse.x);
    root->gd->Light(0, GraphicsDevice::Specular, home_team->light.color.specular.x);
  }

  int Frame(Window *W, unsigned clicks, int flag) {
    W->GetInputController<BindMap>(0)->Repeat(clicks);

    if (Singleton<FlagMap>::Get()->dirty) {
      Singleton<FlagMap>::Get()->dirty = false;
      SettingsFile::Write(save_settings, LFAppDownloadDir(), "settings");
    }

    if (FLAGS_multitouch) {
      static int last_rpad_down = 0;
      bool changed = touchcontrols->rpad_down != last_rpad_down;
      last_rpad_down = touchcontrols->rpad_down;
      if (changed && touchcontrols->rpad_down == GameMultiTouchControls::DOWN) { SwitchPlayerCmd(vector<string>()); }
      if (changed && touchcontrols->rpad_down == GameMultiTouchControls::UP)   { /* release = fire! */ }
      else                                                                     { server->MoveBoost(0); }
    }

    if (server) server->Frame();
#ifdef LFL_BUILTIN_SERVER
    if (builtin_server_enabled) builtin_server->Frame();
#endif

    GraphicsContext gc(root->gd);
    if (map_transition > 0) {
      map_transition -= clicks;
      gc.gd->DrawMode(DrawMode::_2D);
      FLAGS_shadertoy_blend = 1 - float(map_transition) / map_transition_start;
      glShadertoyShaderWindows(gc.gd, &my_app->warpshader, Color::grey60, W->Box(), &framebuffer.tex);
      return 0;

    } else {
      scene.cam.Look(gc.gd);
      shooting_stars.Update(&scene.cam, clicks, 0, 0, 0);
      ball_trail.Update(&scene.cam, clicks, 0, 0, 0);
      if (server->ball) ball_particles->pos = server->ball->pos;

      Scene::EntityVector deleted;
      Scene::LastUpdatedFilter scene_filter_deleted(0, server->last.time_frame, &deleted);

      gc.gd->Light(0, GraphicsDevice::Position, &home_team->light.pos.x);
      skybox.Draw(gc.gd);
      if (draw_skybox_only) return 0;

      // Custom Scene::Draw();
      for (auto &a : my_app->asset.vec) {
        if (a.zsort) continue;
        if (a.name == "lines") {
          scene.Draw(gc.gd, &a);
          gc.gd->EnableDepthTest();
        } else if (a.name == "ball") {
          scene.Draw(gc.gd, &a, &scene_filter_deleted);
        } else {
          scene.Draw(gc.gd, &a);
        }
      }

      scene.ZSort(my_app->asset.vec);
      scene.ZSortDraw(gc.gd, &scene_filter_deleted, clicks);
      server->WorldDeleteEntity(deleted);
    }

    gc.gd->DrawMode(DrawMode::_2D);
    chat->Draw();

    if (FLAGS_multitouch) {
      touchcontrols->Update(clicks);
      touchcontrols->Draw(gc.gd);

      // iPhone keyboard
      // static Font *mobile_font = Fonts::Get("MobileAtlas", "", 0, Color::white, Color::clear, 0);
      // static Widget::Button iPhoneKeyboardButton(W->gui_root, 0, 0, Box::FromScreen(W->multitouch_keyboard_x, .05, .07, .05),
      //                                           MouseController::CB(bind(&Shell::showkeyboard, &app->shell, vector<string>())));
      // iPhoneKeyboardButton.Draw(mobile_font, 5);

      // Game menu and player list buttons
      // static Widget::Button gamePlayerListButton(root, 0, 0, Box::FromScreen(.465, .05, .07, .05), MouseController::CB(bind(&GUI::ToggleDisplay, (GUI*)playerlist)));
      // static Widget::Button           helpButton(root, 0, 0, Box::FromScreen(.56,  .05, .07, .05), MouseController::CB(bind(&GUI::ToggleDisplay, (GUI*)helper)));

      // if (helper && gamePlayerListButton.init) helper->AddLabel(gamePlayerListButton.win, "player list", HelperGUI::Hint::UP, .08);
      // if (helper &&           helpButton.init) helper->AddLabel(          helpButton.win, "help",        HelperGUI::Hint::UPRIGHT);
      // gamePlayerListButton.Draw(mobile_font, 4);
      // helpButton.Draw(mobile_font, 6);
    }

    if (server->replay.enabled()) {
      if (server->ball) {
        // Replay camera tracks ball
        v3 targ = server->ball->pos + server->ball->vel;
        v3 yaw_delta = v3::Norm(targ - v3(scene.cam.pos.x, targ.y, scene.cam.pos.z));
        v3 ort = v3::Norm(targ - scene.cam.pos);
        v3 delta = ort - yaw_delta;
        scene.cam.up = v3(0,1,0) + delta;
        scene.cam.ort = ort;
      }

      Box win(W->width*.4, W->height*.8, W->width*.2, W->height*.1, false);
      Asset *goal = my_app->asset("goal");
      goal->tex.Bind();
      win.Draw(gc.gd, goal->tex.coord);

      static Font *font = app->fonts->Get(FLAGS_font, "", 16);
      font->Draw(StrCat(server->last_scored_PlayerName, " scores"),
                 Box(win.x, win.y - W->height*.1, W->width*.2, W->height*.1, false), 
                 0, Font::DrawFlag::AlignCenter | Font::DrawFlag::NoWrap);

      bool home_team_scored = server->last_scored_team == Game::Team::Home;
      fireworks.rand_color_min = fireworks.rand_color_max = home_team_scored ? home_goal_color : away_goal_color;
      fireworks.rand_color_min.scale(.6);
      for (int i=0; i<Fireworks::MaxParticles; i++) {
        if (fireworks.particles[i].dead) continue;
        fireworks.particles[i].InitColor();
      }
      fireworks_positions[0].set(-W->width*.2, W->height*.8, 0);
      fireworks_positions[1].set(-W->width*.8, W->height*.8, 0);
      fireworks.Update(&scene.cam, clicks, 0, 0, 0);
      fireworks.Draw(gc.gd);
      scene.Select(gc.gd);
    }

    if (server->gameover.enabled()) {
      Box win(W->width*.4, W->height*.9, W->width*.2, W->height*.1, false);
      static Font *font = app->fonts->Get(FLAGS_font, "", 16);
      font->Draw(StrCat(server->gameover.start_ind == SpaceballGame::Team::Home ? home_team->name : away_team->name, " wins"),
                 win, 0, Font::DrawFlag::AlignCenter);
    }

    if (server->replay.just_ended) {
      server->replay.just_ended = false;
      scene.cam.ort = SpaceballGame::StartOrientation(server->team);
      scene.cam.up  = v3(0, 1, 0);
    }

    if (team_select->active) team_select->Draw(my_app->fadershader.ID > 0 ? &my_app->fadershader : 0);

    // Press escape for menubar
    else if (menubar->active) menubar->Draw(clicks, my_app->fadershader.ID > 0 ? &my_app->fadershader : 0);

    // Press 't' to talk
    else if (chat->Active()) {}

    // Press tab for playerlist
    else if (playerlist->active || server->gameover.enabled()) {
      server->control.SetPlayerList();
      playerlist->Draw(my_app->fadershader.ID > 0 ? &my_app->fadershader : 0);
    }

    // Press tick for console
    else W->DrawDialogs();

    Scene::Select(gc.gd);

    if (helper && helper->active) {
      gc.gd->SetColor(helper->font->fg);
      BoxOutline().Draw(&gc, menubar->topbar.box);
      helper->Draw();
    }

    gc.gd->EnableBlend();
    static Font *text = app->fonts->Get(FLAGS_font, "", 8);
    if (FLAGS_draw_fps)   text->Draw(StringPrintf("FPS = %.2f", app->FPS()),           point(W->width*.05, W->height*.05));
    if (!menubar->active) text->Draw(intervalminutes(Now() - map_started),             point(W->width*.93, W->height*.97));
    if (!menubar->active) text->Draw(StrCat(home_team->name, " vs ", away_team->name), point(W->width*.01, W->height*.97));

    return 0;
  }

  void ServerCmd(const vector<string> &arg) {
    DisableLocalServer();
    if (arg.empty()) { INFO("eg: server 192.168.1.144:", FLAGS_default_port); return; }
    server->Connect(default_transport_protocol, arg[0], FLAGS_default_port);
  }

  void LocalServerCmd(const vector<string>&) {
    int game_type = sbsettings.GetIndex(SpaceballSettings::GAME_TYPE);
    if (game_type == SpaceballSettings::TYPE_TOURNAMENT) {
      if (!team_select->active) { team_select->active = true; return; }
      team_select->active = false;
    }
    EnableLocalServer(game_type);
    server->Connect(local_transport_protocol, "127.0.0.1", FLAGS_default_port);
  }

  void GPlusServerCmd(const vector<string> &arg) {
#ifdef LFL_ANDROID
    menubar->Deactivate();
    EnableLocalServer(SpaceballSettings::TYPE_EMPTYCOURT);
    if (arg.empty()) { INFO("eg: gplus_server participant_id"); return; }
    INFO("GPlusServer ", arg[0]);
    AndroidGPlusService(builtin_server->gplus_transport);
    server->Connect(Protocol::GPLUS, "127.0.0.1", FLAGS_default_port);
#endif
  }

  void GPlusClientCmd(const vector<string> &arg) {
#ifdef LFL_ANDROID
    menubar->Deactivate();
    DisableLocalServer();
    if (arg.empty()) { INFO("eg: gplus_client participant_id"); return; }
    INFO("GPlusClient ", arg[0]);
    // android_gplus_service(app->net->gplus_client.get());
    // server->Connect(Protocol::GPLUS, arg[0], 0);
#endif
  }

  void SwitchPlayerCmd(const vector<string> &) { if (server) server->Rcon("player_switch"); }
  void FieldColorCmd(const vector<string> &arg) {
    Color fc(arg.size() ? arg[0] : "");
    Asset *field = my_app->asset("field");
    delete field->geometry;
    field->geometry = FieldGeometry(home_goal_color, away_goal_color, fc);
    INFO("field_color = ", fc.HexString());
  }
};

void MyWindowInit(Window *W) {
  W->caption = "Spaceball 6006";
  W->multitouch_keyboard_x = .37;
}

void MyWindowStart(Window *W) {
  CHECK_EQ(0, W->NewGUI());
  MyGameWindow *game_gui = W->ReplaceGUI(0, make_unique<MyGameWindow>(W));
  W->frame_cb = bind(&MyGameWindow::Frame, game_gui, _1, _2, _3);
  if (FLAGS_console) W->InitConsole(Callback());

  BindMap *binds = W->AddInputController(make_unique<BindMap>());
#if 0                                   
  binds->Add('w',             Bind::TimeCB(bind(&Entity::MoveFwd,       W->camMain, _1)));
  binds->Add('s',             Bind::TimeCB(bind(&Entity::MoveRev,       W->camMain, _1)));
  binds->Add('a',             Bind::TimeCB(bind(&Entity::MoveLeft,      W->camMain, _1)));
  binds->Add('d',             Bind::TimeCB(bind(&Entity::MoveRight,     W->camMain, _1)));
  binds->Add('q',             Bind::TimeCB(bind(&Entity::MoveDown,      W->camMain, _1)));
  binds->Add('e',             Bind::TimeCB(bind(&Entity::MoveUp,        W->camMain, _1)));
#else
  binds->Add('w',             Bind::TimeCB(bind(&GameClient::MoveFwd,   game_gui->server, _1)));
  binds->Add('s',             Bind::TimeCB(bind(&GameClient::MoveRev,   game_gui->server, _1)));
  binds->Add('a',             Bind::TimeCB(bind(&GameClient::MoveLeft,  game_gui->server, _1)));
  binds->Add('d',             Bind::TimeCB(bind(&GameClient::MoveRight, game_gui->server, _1)));
  binds->Add('q',             Bind::TimeCB(bind(&GameClient::MoveDown,  game_gui->server, _1)));
  binds->Add('e',             Bind::TimeCB(bind(&GameClient::MoveUp,    game_gui->server, _1)));
#endif
#ifndef LFL_MOBILE
  binds->Add(Mouse::Button::_1, Bind::TimeCB(bind(&SpaceballClient::MoveBoost, game_gui->server, _1)));
#endif
#ifdef LFL_EMSCRIPTEN
  W->grabbed_pitch_cb = function<void(int)>();
  binds->move_cb = bind(&Entity::MovePitchCB, &game_gui->scene.cam, _1, _2);
#else
  binds->move_cb = bind(&Entity::MoveCB, &game_gui->scene.cam, _1, _2);
  binds->Add(Key::LeftShift,  Bind::TimeCB(bind(&Entity::RollLeft,   &game_gui->scene.cam, _1)));
  binds->Add(Key::Space,      Bind::TimeCB(bind(&Entity::RollRight,  &game_gui->scene.cam, _1)));
#endif
  binds->Add(Key::Tab,        Bind::TimeCB(bind(&GUI::Activate, game_gui->playerlist)));
  binds->Add(Key::F1,         Bind::CB(bind(&GameClient::SetCamera,  game_gui->server,  vector<string>(1, string("1")))));
  binds->Add(Key::F2,         Bind::CB(bind(&GameClient::SetCamera,  game_gui->server,  vector<string>(1, string("2")))));
  binds->Add(Key::F3,         Bind::CB(bind(&GameClient::SetCamera,  game_gui->server,  vector<string>(1, string("3")))));
  binds->Add(Key::F4,         Bind::CB(bind(&GameClient::SetCamera,  game_gui->server,  vector<string>(1, string("4")))));
  binds->Add(Key::Return,     Bind::CB(bind(&Shell::grabmode,        W->shell.get(),    vector<string>())));
  binds->Add('r',             Bind::CB(bind(&MyGameWindow::SwitchPlayerCmd, game_gui, vector<string>()))); 
  binds->Add('t',             Bind::CB(bind([=](){ game_gui->chat->ToggleActive(); })));
  binds->Add(Key::Escape,     Bind::CB(bind([=](){ game_gui->menubar->ToggleActive(); })));
  binds->Add(Key::Backquote,  Bind::CB(bind([=](){ if (!game_gui->menubar->active) W->shell->console(vector<string>()); })));
  binds->Add(Key::Quote,      Bind::CB(bind([=](){ if (!game_gui->menubar->active) W->shell->console(vector<string>()); })));

  W->shell = make_unique<Shell>(&my_app->asset, &my_app->soundasset, nullptr);
  W->shell->Add("server",       bind(&MyGameWindow::ServerCmd,      game_gui, _1));
  W->shell->Add("field_color",  bind(&MyGameWindow::FieldColorCmd,  game_gui, _1));
  W->shell->Add("local_server", bind(&MyGameWindow::LocalServerCmd, game_gui, _1));
  W->shell->Add("gplus_client", bind(&MyGameWindow::GPlusClientCmd, game_gui, _1));
  W->shell->Add("gplus_server", bind(&MyGameWindow::GPlusServerCmd, game_gui, _1));
  W->shell->Add("rcon",         bind(&GameClient::RconCmd,      game_gui->server, _1));
  W->shell->Add("name",         bind(&GameClient::SetName,      game_gui->server, _1));
  W->shell->Add("team",         bind(&GameClient::SetTeam,      game_gui->server, _1));
  W->shell->Add("me",           bind(&GameClient::MyEntityName, game_gui->server, _1));
};

}; // namespace LFL
using namespace LFL;

extern "C" void MyAppCreate(int argc, const char* const* argv) {
  FLAGS_far_plane = 1000;
  FLAGS_soundasset_seconds = 2;
  FLAGS_scale_font_height = 320;
  FLAGS_font_engine = "atlas";
  FLAGS_font = FLAGS_console_font = "Origicide";
  FLAGS_font_flag = FLAGS_console_font_flag = 0;
  FLAGS_enable_audio = FLAGS_enable_video = FLAGS_enable_input = FLAGS_enable_network = FLAGS_console = 1;
  FLAGS_depth_buffer_bits = 16;
  app = new Application(argc, argv);
  screen = new Window();
  my_app = new MyAppState();
#ifdef LFL_MOBILE
  FLAGS_target_fps = 30;
  screen->SetBox(Box(420, 380));
#else
  FLAGS_target_fps = 60;
  screen->SetBox(Box(840, 760));
#endif
  app->name = "Spaceball";
  app->window_start_cb = MyWindowStart;
  app->window_init_cb = MyWindowInit;
  app->window_init_cb(screen);
  app->exit_cb = []{ delete my_app; };
}

extern "C" int MyAppMain() {
  if (app->Create(__FILE__)) return -1;
  if (app->Init()) return -1;
  INFO("BUILD Version ", "1.02.1");

  FontEngine *atlas_engine = app->fonts->atlas_engine.get();
  atlas_engine->Init(FontDesc("MobileAtlas",              "",  0, Color::white, Color::clear, 0, false));
  atlas_engine->Init(FontDesc("dpad_atlas",               "",  0, Color::white, Color::clear, 0, false));
  atlas_engine->Init(FontDesc("sbmaps",                   "",  0, Color::white, Color::clear, 0, false));
  atlas_engine->Init(FontDesc("lightning",                "",  0, Color::white, Color::clear, 0, false));
  atlas_engine->Init(FontDesc(StrCat(FLAGS_font, "Glow"), "", 32, Color::white, Color::clear, 0, false));

  SettingsFile::Read(LFAppDownloadDir(), "settings");
  Singleton<FlagMap>::Get()->dirty = false;
  screen->gd->default_draw_mode = DrawMode::_3D;

  if (FLAGS_player_name.empty()) {
#if defined(LFL_ANDROID)
    char buf[40];
    if (AndroidDeviceName(buf, sizeof(buf))) FLAGS_player_name = buf;
#endif
    if (FLAGS_player_name.empty()) FLAGS_player_name = "n00by";
  }

  if (FLAGS_first_run) {
    INFO("Welcome to Spaceball 6006, New Player.");
    Singleton<FlagMap>::Get()->Set("first_run", "0");
  }

  // my_app->assets.Add(name,     texture,          scale,            trans, rotate, geometry      hull,                            cubemap,     texgen));
  my_app->asset.Add("particles",  "particle.png",   1,                0,     0,      nullptr,      nullptr,                         0,           0);
  my_app->asset.Add("stars",      "",               1,                0,     0,      nullptr,      nullptr,                         0,           0);
  my_app->asset.Add("glow",       "glow.png",       1,                0,     0,      nullptr,      nullptr,                         0,           0);
  my_app->asset.Add("field",      "",               1,                0,     0,      nullptr,      nullptr,                         0,           0);
  my_app->asset.Add("lines",      "lines.png",      1,                0,     0,      nullptr,      nullptr,                         0,           0);
  my_app->asset.Add("ball",       "",               MyBall::radius(), 1,     0,      "sphere.obj", nullptr,                         0);
  my_app->asset.Add("ship",       "ship.png",       .05,              1,     0,      "ship.obj",   Cube::Create(MyShip::radius()).release(), 0, &ShipDraw);
  my_app->asset.Add("shipred",    "",               0,                0,     0,      nullptr,      nullptr,                         0,           0);
  my_app->asset.Add("shipblue",   "",               0,                0,     0,      nullptr,      nullptr,                         0,           0);
  my_app->asset.Add("title",      "title.png",      1,                0,     0,      nullptr,      nullptr,                         0,           0);
  my_app->asset.Add("goal",       "goal.png",       1,                0,     0,      nullptr,      nullptr,                         0,           0);
  my_app->asset.Load();
  my_app->asset("field")->blendt = GraphicsDevice::SrcAlpha;
  Asset *lines = my_app->asset("lines");
  lines->geometry = FieldLines(lines->tex.coord[2], lines->tex.coord[3]);
  Asset::LoadTextureArray("%s%02d.%s", "caust", "png", 32, &my_app->caust, VideoAssetLoader::Flag::Default | VideoAssetLoader::Flag::RepeatGL);

  if (FLAGS_enable_audio) {
    // soundasset.Add(name,  filename,              ringbuf, channels, sample_rate, seconds );
    my_app->soundasset.Add("music",  "dstsecondballad.ogg", nullptr, 0,        0,           0       );
    my_app->soundasset.Add("bounce", "scififortyfive.wav",  nullptr, 0,        0,           0       );
    my_app->soundasset.Load();
    auto bounce = my_app->soundasset("bounce");
    bounce->max_distance = 1000;
    bounce->reference_distance = SpaceballGame::FieldDefinition::get()->Length() / 3.0;
  }

  if (screen->gd->ShaderSupport()) {
    string fader_shader  = Asset::FileContents("fader.frag");
    string warp_shader = Asset::FileContents("warp.frag");
    string explode_shader = screen->gd->vertex_shader;
    CHECK(ReplaceString(&explode_shader, "// LFLPositionShaderMarker",
                        Asset::FileContents("explode.vert")));

    Shader::Create("fadershader",   screen->gd->vertex_shader, fader_shader,             "",                     &my_app->fadershader);
    Shader::Create("explodeshader", explode_shader,            screen->gd->pixel_shader, ShaderDefines(0,1,1,0), &my_app->explodeshader);
    Shader::CreateShaderToy("warpshader", warp_shader, &my_app->warpshader);
  }

  // Color ships red and blue
  Asset *ship = my_app->asset("ship"), *shipred = my_app->asset("shipred"), *shipblue = my_app->asset("shipblue");
  ship->particleTexID = my_app->asset("glow")->tex.ID;
  ship->zsort = true;
  Asset::Copy(ship, shipred);
  Asset::Copy(ship, shipblue);
  shipred->color = shipblue->color = true;

  app->StartNewWindow(screen);
  MyGameWindow *game_gui = screen->GetOwnGUI<MyGameWindow>(0);

  // add reflection to ball
  Asset *ball = my_app->asset("ball"), *sky = game_gui->skybox.asset();
  if (sky->tex.cubemap) {
    ball->tex.ID = sky->tex.ID;
    ball->tex.cubemap = sky->tex.cubemap;
    ball->texgen = TexGen::LINEAR;
    ball->geometry->material = 0;
    ball->color = true;
    ball->col = Color(1.4, 1.4, 1.4);
  }

  Game::Credits *credits = Singleton<Game::Credits>::Get();
  credits->emplace_back("Game",         "koldfuzor",    "http://www.lucidfusionlabs.com/", "");
  credits->emplace_back("Skyboxes",     "3delyvisions", "http://www.3delyvisions.com/",    "");
  credits->emplace_back("Physics",      "Box2D",        "http://box2d.org/",               "");
  credits->emplace_back("Fonts",        "Cpr.Sparhelt", "http://www.facebook.com/pages/Magique-Fonts-Koczman-B%C3%A1lint/110683665690882", "");
  credits->emplace_back("Image format", "Libpng",       "http://www.libpng.org/",          "");

  if (FLAGS_enable_audio) app->PlayBackgroundMusic(my_app->soundasset("music"));
  return app->Main();
}
