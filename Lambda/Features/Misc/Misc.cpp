#include "Misc.h"
#include "../RageBot/Exploits.h"
#include "Prediction.h"
#include "../../Utils/Utils.h"
#include "../../SDK/Interfaces.h"
#include "../../SDK/Config.h"
#include "../../SDK/Globals.h"

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#include "../../Utils/json.hpp"
#include "../../SDK/Render.h"
#include "../../UI/UI.h"

void Miscellaneous::Clantag()
{
	if (!Menu->IsInitialized())
		return;

	static auto removed = false;

	if (!config.misc.miscellaneous.clantag->get() && !removed)
	{
		removed = true;
		Utils::SetClantag("");
		return;
	}

	if (config.misc.miscellaneous.clantag->get())
	{
		if (!EngineClient->IsInGame())
			return;

		static auto time = -1;

		const int server_tick = Cheat.LocalPlayer ? Cheat.LocalPlayer->m_nTickBase() : GlobalVars->tickcount;
		const int ticks_per_frame = (int)(0.4f / GlobalVars->interval_per_tick);

		auto main_time = (server_tick / (ticks_per_frame > 0 ? ticks_per_frame : 1)) % 24;

		if (main_time != time && !ClientState->m_nChokedCommands)
		{
			static const char* frames[] = {
				">_____", "L>____", "La>___", "Lam>__",
				"Lamb>_", "Lambd>", "Lambda", ">ambda",
				"_>mbda", "__>bda", "___>da", "____>a",
				"_____<", "____<a", "___<da", "__<bda",
				"_<mbda", "<ambda", "Lambda", "Lambd<",
				"Lamb<_", "Lam<__", "La<___", "L<____",
			};

			Utils::SetClantag(frames[main_time]);
			time = main_time;
		}

		removed = false;
	}
}

static const char* trashtalk_kill_phrases[] = {
	"discord.gg/lmbda stop using your shitty cheat dog",
	"Maybe if you used lambda you wouldnt stress",
	"your config is the only thing holding you back",
	"1, maybe 2, maybe 3, who knows? you still died",
	"lmao Stop wining and just get lambda at gg/lmbda (free)",
	"sigh another round another kill, i wonder when something will change",
	"watch how i flex",
	"oh btw i use lambda, its free nd open source, check it out. gg/lmbda",
	"who the fuck made these killsays??? -- idk maybe its n1kvz?",
	"even rifk7 is better than lambda, and your losing",
	"hmm what to say?? OH! 1.",
	"maybe if you tried lambda you would find hvh more challenging, but i can see it already is for you...",
	"sigh im so bored of this maybe ill change servers",
};

static const char* trashtalk_death_phrases[] = {
	"Today isnt the users day, please turn off trashtalk for him?",
	"The only lambda user died, what a suprise",
	"I guess its easy to die now, good job user",
	"Welp i might aswell plug lambda. gg/lmbda",
	"didnt think much of a open sourced cheat anyway..",
	"soo when are you gonna fix config?",
	"I wonder what i did wrong, oh thats right! im a fucking idiot",
	"this is getting really embarrasing, up your game dumbass user",
	"the user is probably negative kd right now or am i wrong.",
	"soo when am i gonna be able to be used properly, because the user obviously doesnt know how to haha.",
};
static std::vector<std::string> trashtalk_pending;

static int TrashtalkRandom(int limit)
{
	static auto state = 0u;

	if (!state)
		state = (unsigned int)GetTickCount() | 1u;

	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;

	return (int)(state % (unsigned int)limit);
}

void Miscellaneous::Trashtalk(bool killed)
{
	if (!config.misc.miscellaneous.trashtalk->get())
		return;

	const auto event = killed ? 0 : 1;

	if (!config.misc.miscellaneous.trashtalk_events->get(event))
		return;

	auto phrases = killed ? trashtalk_kill_phrases : trashtalk_death_phrases;
	auto count = killed ? (int)_countof(trashtalk_kill_phrases) : (int)_countof(trashtalk_death_phrases);

	if (count < 1)
		return;

	static int previous[2] = { -1, -1 };

	auto index = 0;

	if (count > 1)
	{
		if (previous[event] < 0 || previous[event] >= count)
			index = TrashtalkRandom(count);
		else
		{
			index = TrashtalkRandom(count - 1);

			if (index >= previous[event])
				++index;
		}
	}

	previous[event] = index;

	trashtalk_pending.emplace_back(phrases[index]);
}

void Miscellaneous::FlushTrashtalk()
{
	if (trashtalk_pending.empty())
		return;

	if (!EngineClient->IsInGame())
	{
		trashtalk_pending.clear();
		return;
	}

	for (auto& phrase : trashtalk_pending)
		EngineClient->ExecuteClientCmd(("say \"" + phrase + "\"").c_str());

	trashtalk_pending.clear();
}

void Miscellaneous::FastThrow() {
	static bool fast_throw_triggred = false;
	static int nLastButtons = 0;

	if (!ctx.active_weapon || !ctx.active_weapon->IsGrenade()) {
		fast_throw_triggred = false;
		return;
	}

	CBaseGrenade* grenade = reinterpret_cast<CBaseGrenade*>(ctx.active_weapon);

	if (!(ctx.cmd->buttons & (IN_ATTACK | IN_ATTACK2)) && (nLastButtons & (IN_ATTACK | IN_ATTACK2)) && grenade->m_bPinPulled())
		ctx.grenade_throw_tick = ctx.cmd->command_number;

	nLastButtons = ctx.cmd->buttons;

	if (!config.ragebot.aimbot.doubletap_options->get(3)) {
		fast_throw_triggred = false;
		return;
	}

	if (ctx.tickbase_shift > 0) {
		Exploits->LC_OverrideTickbase(ctx.tickbase_shift);

		float arm_time = max(Cheat.LocalPlayer->m_flNextAttack(), grenade->m_flNextPrimaryAttack());

		if (TICKS_TO_TIME(Cheat.LocalPlayer->m_nTickBase()) + TICKS_TO_TIME(ctx.tickbase_shift - 7) > arm_time)
			Exploits->LC_OverrideTickbase(7);

		if (grenade->m_flThrowTime() > 0.f)
			fast_throw_triggred = true;

		if (fast_throw_triggred)
			Exploits->LC_OverrideTickbase(0);
	}

	if (ctx.cmd->command_number == ctx.grenade_throw_tick + 8 && ctx.grenade_throw_tick != 0)
		ctx.switch_to_main_weapon = true;
}


void Miscellaneous::FastSwitch() {
	if (!ctx.switch_to_main_weapon)
		return;

	ctx.switch_to_main_weapon = false;
	CBaseCombatWeapon* best_weapon = nullptr;
	auto weapons = Cheat.LocalPlayer->m_hMyWeapons();
	int best_type = WEAPONTYPE_KNIFE;
	for (int i = 0; i < MAX_WEAPONS; i++) {
		auto weap = weapons[i];
		if (weap == INVALID_EHANDLE_INDEX)
			break;

		CBaseCombatWeapon* weapon = reinterpret_cast<CBaseCombatWeapon*>(EntityList->GetClientEntityFromHandle(weap));

		if (!weapon)
			continue;

		CCSWeaponData* weap_info = weapon->GetWeaponInfo();

		if (!weap_info)
			continue;

		if (weap_info->nWeaponType >= WEAPONTYPE_SUBMACHINEGUN && weap_info->nWeaponType <= WEAPONTYPE_MACHINEGUN) {
			best_weapon = weapon;
			best_type = weap_info->nWeaponType;
		}
		else if (weap_info->nWeaponType == WEAPONTYPE_PISTOL && best_type == WEAPONTYPE_KNIFE) {
			best_weapon = weapon;
			best_type = weap_info->nWeaponType;
		}
	}

	if (best_weapon)
		ctx.cmd->weaponselect = best_weapon->EntIndex();
}

void Miscellaneous::AutomaticGrenadeRelease() {
	static bool prev_release = false;
	static Vector on_release_move;
	static QAngle on_release_angle;

	if (ctx.should_release_grenade && ctx.active_weapon && ctx.active_weapon->IsGrenade()) {
		if (!prev_release) {
			on_release_move = Vector(ctx.cmd->sidemove, ctx.cmd->forwardmove);
			on_release_angle = ctx.grenade_release_angle;
		}

		ctx.cmd->buttons &= ~(IN_ATTACK | IN_ATTACK2);

		if (ctx.cmd->command_number <= ctx.grenade_throw_tick + 7) {
			ctx.cmd->sidemove = on_release_move.x;
			ctx.cmd->forwardmove = on_release_move.y;
			ctx.cmd->viewangles = on_release_angle;
		}
	}
	else if (!ctx.active_weapon || !ctx.active_weapon->IsGrenade()) {
		ctx.should_release_grenade = false;
	}

	prev_release = ctx.should_release_grenade;
}

static bool s_ShouldClearNotices = false;
void Miscellaneous::PreserveKillfeed() {
	if (!Cheat.InGame || !Cheat.LocalPlayer)
		return;

	static auto spawntime = 0.f;
	static auto status = false;
	static auto clear_deathnotices = reinterpret_cast<void(__thiscall*)(uintptr_t*)>(Utils::PatternScan("client.dll", "55 8B EC 83 EC 0C 53 56 8B 71 58"));

	auto set = false;
	if (spawntime != Cheat.LocalPlayer->m_flSpawnTime() || status != config.visuals.effects.preserve_killfeed->get())
	{
		set = true;
		status = config.visuals.effects.preserve_killfeed->get();
		spawntime = Cheat.LocalPlayer->m_flSpawnTime();
	}

	const auto hud_radar = CSGOHud->FindHudElement("CCSGO_HudRadar");
	const auto death_notice = reinterpret_cast<uintptr_t>(CSGOHud->FindHudElement("CCSGO_HudDeathNotice"));
	if (death_notice == 20)
		return;

	const auto notice_element = reinterpret_cast<uintptr_t*>(death_notice - 0x14);
	if (!death_notice || !notice_element)
		return;

	if (set) {
		const auto lifetime = reinterpret_cast<float*>(death_notice + 0x50);
		*lifetime = status ? FLT_MAX : 1.5f;
	}

	if (s_ShouldClearNotices) {
		s_ShouldClearNotices = false;
		clear_deathnotices(notice_element);
	}
}

void Miscellaneous::ClearKillfeed() {
	s_ShouldClearNotices = true;
}

void Miscellaneous::RadarAngles() {
	if (!Cheat.InGame)
		return;

	static const auto hud_radar = reinterpret_cast<CSGO_HudRadar*>(CSGOHud->FindHudElement("CCSGO_HudRadar"));

	if (!hud_radar)
		return;

	hud_radar->m_vecLocalAngles.y = 0.f;
}

void Miscellaneous::FPSBoost() {
	static ConVar* r_shadows             = CVar->FindVar("r_shadows");
	static ConVar* r_drawparticles       = CVar->FindVar("r_drawparticles");
	static ConVar* cl_csm_shadows        = CVar->FindVar("cl_csm_shadows");
	static ConVar* cl_foot_contact_shad  = CVar->FindVar("cl_foot_contact_shadows");
	static ConVar* mat_disable_bloom     = CVar->FindVar("mat_disable_bloom");
	static ConVar* r_eyegloss            = CVar->FindVar("r_eyegloss");
	static ConVar* r_eyemove             = CVar->FindVar("r_eyemove");
	static ConVar* dsp_slow_cpu          = CVar->FindVar("dsp_slow_cpu");

	static bool prev_state = false;
	bool enabled = config.misc.miscellaneous.fps_boost->get();

	if (enabled == prev_state)
		return;

	prev_state = enabled;

	if (enabled) {
		if (r_shadows)            r_shadows->SetInt(0);
		if (r_drawparticles)      r_drawparticles->SetInt(0);
		if (cl_csm_shadows)       cl_csm_shadows->SetInt(0);
		if (cl_foot_contact_shad) cl_foot_contact_shad->SetInt(0);
		if (mat_disable_bloom)    mat_disable_bloom->SetInt(1);
		if (r_eyegloss)           r_eyegloss->SetInt(0);
		if (r_eyemove)            r_eyemove->SetInt(0);
		if (dsp_slow_cpu)         dsp_slow_cpu->SetInt(1);
	} else {
		if (r_shadows)            r_shadows->SetInt(1);
		if (r_drawparticles)      r_drawparticles->SetInt(1);
		if (cl_csm_shadows)       cl_csm_shadows->SetInt(1);
		if (cl_foot_contact_shad) cl_foot_contact_shad->SetInt(1);
		if (mat_disable_bloom)    mat_disable_bloom->SetInt(0);
		if (r_eyegloss)           r_eyegloss->SetInt(1);
		if (r_eyemove)            r_eyemove->SetInt(1);
		if (dsp_slow_cpu)         dsp_slow_cpu->SetInt(0);
	}
}

 //\\ --obfuscated shit you dont need (its for shared features)
namespace {

constexpr unsigned char _sh_k[] = { 0xA3, 0x5F, 0x27, 0x91, 0xC4, 0xD8, 0x6B, 0x3E, 0xF1, 0x7A, 0x02, 0x5C, 0xE9, 0x84, 0x1D, 0xB6 };
constexpr unsigned char _sh_h[] = { 0x9A, 0x6B, 0x1F, 0xA6, 0xF5, 0xEC, 0x13, 0x1B, 0xD6, 0x4A, 0x24, 0x6C, 0xD2, 0xB2, 0x2D, 0xCD, 0x91, 0x78, 0x13, 0xA6, 0xF8, 0xE3, 0x4D, 0x00, 0xC5, 0x5C, 0x7A, 0x3B, 0x89, 0xFF, 0x3F, 0x8C, 0x84, 0x61, 0x17, 0xB6, 0xE2, 0xA3, 0x5A, 0x0E, 0xD2 };
constexpr unsigned char _sh_kv_b[] = { 0x9D, 0x7E, 0x1F, 0x81, 0xC9, 0xD9, 0x7B, 0x02, 0xF7, 0x1D, 0x14, 0x71, 0xD4, 0xE3, 0x23, 0xAD, 0x92, 0x59, 0x1D, 0x94, 0xD0, 0xB9, 0x64, 0x3B, 0xCE, 0x49, 0x25, 0x7D, 0xFE, 0xB9, 0x7A, 0x99, 0x98, 0x59, 0x42, 0xA9, 0xD5, 0xC9, 0x7F, 0x5E };
constexpr int _sh_h_n = 41;
constexpr int _sh_kv_n = 40;

static std::wstring _g_sh_h;
static std::string _g_sh_k;

static const wchar_t* _sh_hv() {
	if (!_g_sh_h.empty()) return _g_sh_h.c_str();
	for (int i = 0; i < _sh_h_n; i++) _g_sh_h += (wchar_t)(_sh_h[i] ^ _sh_k[i % 16] ^ 0x55);
	return _g_sh_h.c_str();
}

static const char* _sh_kvs() {
	if (!_g_sh_k.empty()) return _g_sh_k.c_str();
	for (int i = 0; i < _sh_kv_n; i++) _g_sh_k += (char)(_sh_kv_b[i] ^ _sh_k[i % 16] ^ 0x55);
	return _g_sh_k.c_str();
}

std::mutex g_sharedMutex;
std::vector<std::string> g_sharedNames;
std::atomic<bool> g_sharedNetBusy = false;
float g_sharedLastBeat = -1000.f;
float g_sharedLastFetch = -1000.f;

std::string SharedJsonEscape(const std::string& s)
{
	std::string out;
	out.reserve(s.size());
	for (char c : s) {
		if (c == '"' || c == '\\') out += '\\';
		out += c;
	}
	return out;
}

std::wstring SharedToWide(const std::string& s)
{
	return std::wstring(s.begin(), s.end());
}

std::string SharedHttp(const wchar_t* method, const std::wstring& path, const std::string& body)
{
	std::string result;
	HINTERNET hSession = WinHttpOpen(L"Lambda/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
	if (!hSession) return result;
	HINTERNET hConnect = WinHttpConnect(hSession, _sh_hv(), INTERNET_DEFAULT_HTTPS_PORT, 0);
	if (!hConnect) { WinHttpCloseHandle(hSession); return result; }
	HINTERNET hReq = WinHttpOpenRequest(hConnect, method, path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
	if (!hReq) { WinHttpCloseHandle(hConnect); WinHttpCloseHandle(hSession); return result; }
	WinHttpAddRequestHeaders(hReq, L"Content-Type: application/json\r\n", (ULONG)-1, WINHTTP_ADDREQ_FLAG_ADD);
	if (WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)(body.empty() ? nullptr : body.data()), (DWORD)body.size(), (DWORD)body.size(), 0)
		&& WinHttpReceiveResponse(hReq, nullptr)) {
		DWORD avail = 0;
		while (WinHttpQueryDataAvailable(hReq, &avail) && avail) {
			std::string chunk((size_t)avail, '\0');
			DWORD read = 0;
			if (!WinHttpReadData(hReq, chunk.data(), avail, &read) || !read) break;
			result.append(chunk.data(), read);
		}
	}
	WinHttpCloseHandle(hReq);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
	return result;
}
}

void Miscellaneous::SharedFeatures()
{
	const bool enabled = config.misc.miscellaneous.shared_features->get();
	static bool s_wasEnabled = false;

	if (!enabled) {
		if (s_wasEnabled) {
			s_wasEnabled = false;
			auto nci = EngineClient->GetNetChannelInfo();
			std::string server = (nci && nci->GetAddress()) ? nci->GetAddress() : "?";
			std::string name;
			if (Cheat.LocalPlayer) {
				player_info_t info{};
				if (EngineClient->GetPlayerInfo(Cheat.LocalPlayer->EntIndex(), &info))
					name = info.szName;
			}
			{
				std::lock_guard<std::mutex> lock(g_sharedMutex);
				g_sharedNames.clear();
			}
			g_sharedLastBeat = -1000.f;
			g_sharedLastFetch = -1000.f;
			if (!name.empty() && server != "?" && !g_sharedNetBusy.load()) {
				g_sharedNetBusy = true;
				std::thread([server, name]() {
					std::string body = std::string("{\"key\":\"") + _sh_kvs() + "\",\"name\":\"" + SharedJsonEscape(name) + "\",\"server\":\"" + SharedJsonEscape(server) + "\",\"leave\":true}";
					SharedHttp(L"POST", L"/presence", body);
					g_sharedNetBusy = false;
				}).detach();
			}
		}
		return;
	}
	s_wasEnabled = true;
	if (!Cheat.InGame || !Cheat.LocalPlayer || !Cheat.LocalPlayer->IsAlive())
		return;
	if (!EngineClient->IsInGame())
		return;

	const float now = GlobalVars->realtime;
	if (now - g_sharedLastBeat < 60.f || g_sharedNetBusy.load())
		return;
	g_sharedLastBeat = now;

	auto nci = EngineClient->GetNetChannelInfo();
	if (!nci || !nci->GetAddress())
		return;
	std::string server = nci->GetAddress();

	player_info_t info{};
	if (!EngineClient->GetPlayerInfo(Cheat.LocalPlayer->EntIndex(), &info))
		return;
	std::string name = info.szName;
	if (name.empty())
		return;

	g_sharedNetBusy = true;
	std::thread([server, name]() {
		std::string body = std::string("{\"key\":\"") + _sh_kvs() + "\",\"name\":\"" + SharedJsonEscape(name) + "\",\"server\":\"" + SharedJsonEscape(server) + "\"}";
		SharedHttp(L"POST", L"/presence", body);
		g_sharedNetBusy = false;
	}).detach();
}

void Miscellaneous::DrawSharedFeatures()
{
	if (!config.misc.miscellaneous.shared_features->get())
		return;
	if (!Cheat.InGame || !Cheat.LocalPlayer)
		return;
	if (!Render || !Render->IsInitialized())
		return;
	if (!pic::logo_small.texture)
		return;
	if (!(GetAsyncKeyState(VK_TAB) & 0x8000))
		return;

	const float now = GlobalVars->realtime;
	if (now - g_sharedLastFetch > 5.f && !g_sharedNetBusy.load()) {
		g_sharedLastFetch = now;
		auto nci = EngineClient->GetNetChannelInfo();
		std::string server = (nci && nci->GetAddress()) ? nci->GetAddress() : "?";
		g_sharedNetBusy = true;
		std::thread([server]() {
			std::wstring path = std::wstring(L"/players?key=") + SharedToWide(_sh_kvs()) + L"&server=" + SharedToWide(server);
			std::string resp = SharedHttp(L"GET", path, {});
			std::vector<std::string> names;
			try {
				auto j = nlohmann::json::parse(resp);
				if (j.is_array()) {
					for (auto& e : j) {
						if (e.contains("name") && e["name"].is_string())
							names.emplace_back(e["name"].get<std::string>());
				}
			}
			} catch (...) {}
			std::lock_guard<std::mutex> lock(g_sharedMutex);
			g_sharedNames = std::move(names);
			g_sharedNetBusy = false;
		}).detach();
	}

	std::vector<std::string> lambda_users;
	{
		std::lock_guard<std::mutex> lock(g_sharedMutex);
		lambda_users = g_sharedNames;
	}

	player_info_t self{};
	if (EngineClient->GetPlayerInfo(Cheat.LocalPlayer->EntIndex(), &self))
		lambda_users.push_back(std::string(self.szName));

	if (lambda_users.empty())
		return;

	struct SBPlayer_t {
		std::string name;
		int team;
	};

	std::vector<SBPlayer_t> ct_players, t_players;

	for (int i = 1; i <= GlobalVars->max_clients; i++) {
		CBasePlayer* pl = (CBasePlayer*)EntityList->GetClientEntity(i);
		if (!pl)
			continue;

		player_info_t pi{};
		if (!EngineClient->GetPlayerInfo(i, &pi))
			continue;

		SBPlayer_t entry{ std::string(pi.szName), pl->m_iTeamNum() };

		if (pl->m_iTeamNum() == TEAM_CT)
			ct_players.push_back(entry);
		else if (pl->m_iTeamNum() == TEAM_TERRORIST)
			t_players.push_back(entry);
	}

	const float sw = Cheat.ScreenSize.x;
	const float sh = Cheat.ScreenSize.y;

	const float scale    = sw / 960.f;
	const float row_h    = 26.f  * scale;
	const float icon_sz  = 14.f  * scale;

	const float ct_start_y = 143.f * scale;
	const float t_start_y  = 371.f * scale;

	const float icon_x = 192.f * scale;

	auto draw_for_team = [&](std::vector<SBPlayer_t>& players, float start_y) {
		float y = start_y;
		for (auto& p : players) {
			bool is_lambda = false;
			for (auto& lu : lambda_users) {
				if (lu == p.name) { is_lambda = true; break; }
			}

			if (is_lambda) {
				Render->Image(pic::logo_small,
					Vector2(icon_x, y + (row_h - icon_sz) * 0.5f),
					Color(255, 255, 255, 230));
			}

			y += row_h;
		}
	};

	draw_for_team(ct_players, ct_start_y);
	draw_for_team(t_players,  t_start_y);
}
