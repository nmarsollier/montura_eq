function mountApp() {
    return {
        status: {ra: '--:--:--', dec: '--:--:--'},
        lst: '--:--:--',
        pierSide: '--',
        statusText: 'UNKNOWN',
        settings: {lat: 0, lon: 0, elevation: 0},
        mountTime: '--',
        timeAutoSet: false,
        errorMessage: '',
        errorTimer: null,
        slew: {degrees: 5, speed: 4},
        wifi: {ssid: '', password: ''},
        slewTarget: {ra: {h: 0, m: 0, s: 0}, dec: {d: 0, m: 0, s: 0}, speed: 4},
        serverTracking: 'none',
        wifiAp: false,
        isHome: false,
        isParked: false,
        selectedTracking: null,
        trackingOptions: ['none', 'sidereal', 'lunar', 'solar'],

        /* Show an error message for 5 seconds, then auto-clear. */
        showError(msg) {
            this.errorMessage = msg;
            if (this.errorTimer) clearTimeout(this.errorTimer);
            this.errorTimer = setTimeout(() => {
                this.errorMessage = '';
            }, 5000);
        },

        /* Helper: POST JSON, parse response, show errors automatically. */
        apiPost(url, body) {
            return fetch(url, {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(body)
            }).then(r => r.json()).then(j => {
                if (!j.ok) this.showError(j.message || 'Error desconocido');
                return j;
            }).catch(e => {
                this.showError('Red o servidor no disponible');
                throw e;
            });
        },

        fetchStatus() {
            fetch('/api/status').then(r => r.json()).then(j => {
                this.statusText = j.status || 'UNKNOWN';
                this.status = {ra: j.ra || '--:--:--', dec: j.dec || '--:--:--'};
                this.lst = j.lst || '--:--:--';
                this.pierSide = j.pier_side || '--';
                this.mountTime = j.time || '--';
                this.serverTracking = j.tracking || 'none';
                this.wifiAp = j.wifi_ap || false;
                this.isHome = j.is_home || false;
                this.isParked = (j.status === 'PARKED');

                const s = j.settings;
                if (s) {
                    this.settings.lat = s.lat != null ? s.lat : 0;
                    this.settings.lon = s.lon != null ? s.lon : 0;
                    this.settings.elevation = s.elevation != null ? s.elevation : 0;
                }

                if (!this.timeAutoSet && j.time && j.time < '2000-01-01') {
                    this.timeAutoSet = true;
                    this.updateTime();
                }

                if (this.selectedTracking === null) {
                    const idx = this.trackingOptions.indexOf(j.tracking);
                    this.selectedTracking = idx >= 0 ? idx : 0;
                }
            }).catch(() => {
            });
        },

        stop() {
            this.apiPost('/api/stop').then(() => this.fetchStatus());
        },
        home() {
            this.apiPost('/api/home').then(() => this.fetchStatus());
        },
        park() {
            this.apiPost('/api/park').then(() => this.fetchStatus());
        },
        unpark() {
            this.apiPost('/api/unpark').then(() => this.fetchStatus());
        },

        setTracking() {
            this.apiPost('/api/tracking', {tracking: this.trackingOptions[this.selectedTracking]})
                .then(() => this.fetchStatus());
        },

        // --- Joystick (continuous move via /api/move-axis-speed) ---
        joyRates: {ra: 0, dec: 0},

        joyStart(axis, dir) {
            const dps = 16.0;  // speed profile 4
            this.joyRates[axis] = dir * dps;
            const body = {ra_rate: this.joyRates.ra, dec_rate: this.joyRates.dec};
            this.apiPost('/api/move-axis-speed', body);
        },

        joyStop(axis) {
            this.joyRates[axis] = 0;
            const body = {ra_rate: this.joyRates.ra, dec_rate: this.joyRates.dec};
            this.apiPost('/api/move-axis-speed', body);
        },

        doSlew(axis, dir) {
            const axisName = axis === 'ra' ? 'ra' : 'dec';
            const deg = Math.min(180, Math.max(1, Math.round(Math.abs(this.slew.degrees || 5))));
            const spd = this.slew.speed || 4;
            this.apiPost('/api/move-axis', {axis: axisName, degrees: dir * deg, speed: spd})
                .then(() => this.fetchStatus());
        },

        doSlewToCoordinates() {
            const rh = this.slewTarget.ra.h;
            const rm = this.slewTarget.ra.m;
            const rs = this.slewTarget.ra.s;
            const dd = this.slewTarget.dec.d;
            const dm = this.slewTarget.dec.m;
            const ds = this.slewTarget.dec.s;

            if (isNaN(rh) || rh < 0 || rh > 23 || isNaN(rm) || rm < 0 || rm > 59 || isNaN(rs) || rs < 0 || rs >= 60) {
                this.showError('RA inválida — H: 0–23, M: 0–59, S: 0–59.999');
                return;
            }
            if (isNaN(dd) || dd < -180 || dd > 180 || isNaN(dm) || dm < 0 || dm > 59 || isNaN(ds) || ds < 0 || ds >= 60) {
                this.showError('DEC inválida — D: −180…+180, M: 0–59, S: 0–59.999');
                return;
            }

            const raHours = rh + rm / 60 + rs / 3600;
            const absDec = Math.abs(dd) + dm / 60 + ds / 3600;
            const decDeg = dd < 0 ? -absDec : absDec;

            this.apiPost('/api/slew-to-coordinates', {ra: raHours, dec: decDeg, speed: this.slewTarget.speed})
                .then(() => this.fetchStatus());
        },

        updateTime() {
            const now = new Date().toISOString();
            this.apiPost('/api/settings', {
                time: now,
                lat: this.settings.lat || 0,
                lon: this.settings.lon || 0,
                elevation: this.settings.elevation || 0
            }).then(() => this.fetchStatus());
        },

        updateSettings() {
            this.apiPost('/api/settings', {
                lat: this.settings.lat || 0,
                lon: this.settings.lon || 0,
                elevation: this.settings.elevation || 0
            }).then(() => this.fetchStatus());
        },

        init() {
            this.fetchStatus();
            setInterval(() => this.fetchStatus(), 1000);
        }
    }
}
