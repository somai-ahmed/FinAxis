"""Detection d'anomalies : Benford, doublons, montants ronds, valeurs aberrantes."""
from __future__ import annotations

import ctypes

from bindings.fincore_ffi import ErreurFinCore
from bindings.marshaling import MONNAIE_UNITE, decode
from bindings.structs import CConfigDetection, CRapportBenford, CResultatDetection
from models.detection_result import (BenfordDigit, BenfordReport, DetectionConfig,
                                     DetectionResult, Gravite, MethodeDetection)

from .session_service import SessionService

ERR_ECHANTILLON_BENFORD_INSUFFISANT = -503


class DetectionService:
    def __init__(self, session: SessionService):
        self.session = session

    @property
    def _ffi(self):
        return self.session.ffi

    def config_par_defaut(self) -> DetectionConfig:
        c = CConfigDetection()
        self._ffi.detection_config_par_defaut(ctypes.byref(c))
        return DetectionConfig(bool(c.benford_est_active), bool(c.doublons_est_active), bool(c.nombres_ronds_active),
                               bool(c.valeurs_aberrantes_active), c.seuil_ecart_type_extreme,
                               c.seuil_ecart_nombres_ronds, c.fenetre_jours_doublon)

    @staticmethod
    def _config_vers_c(cfg: DetectionConfig) -> CConfigDetection:
        return CConfigDetection(int(cfg.benford), int(cfg.doublons), int(cfg.nombres_ronds), int(cfg.valeurs_aberrantes),
                                cfg.seuil_ecart_type, cfg.seuil_nombre_rond, cfg.fenetre_jours_doublon)

    def executer(self, id_periode: int, config: DetectionConfig | None = None) -> list[DetectionResult]:
        """Lance les methodes activees ; resultats tries du plus grave au moins grave."""
        cfg = self._config_vers_c(config or self.config_par_defaut())
        sortie = ctypes.POINTER(CResultatDetection)()
        nb = ctypes.c_size_t()
        self._ffi.verifier(self._ffi.execute_detection(self.session.exiger_session(), id_periode,
                                                       ctypes.byref(cfg), ctypes.byref(sortie), ctypes.byref(nb)))
        try:
            resultats = [DetectionResult(MethodeDetection(r.methode), Gravite(r.gravite), r.id_compte, r.id_ligne,
                                         decode(r.description), r.score)
                         for r in (sortie[i] for i in range(nb.value))]
        finally:
            if sortie:
                self._ffi.liberer_resultats_detection(sortie, nb.value)    # memoire allouee par le C : on la rend
        return sorted(resultats, key=lambda r: (-int(r.gravite), -r.score))

    def analyse_benford(self, id_periode: int) -> BenfordReport | None:
        """Rapport statistique de Benford ; None si l'echantillon est trop petit."""
        rapport = CRapportBenford()
        etat = self._ffi.Executer_Analyse_Benford(self.session.exiger_session(), id_periode, ctypes.byref(rapport))
        if etat in (ERR_ECHANTILLON_BENFORD_INSUFFISANT, -501):
            return None
        self._ffi.verifier(etat)
        chiffres = [BenfordDigit(s.numero, s.freq_attendu, s.freq_observe, s.nombre_observe) for s in rapport.stat]
        return BenfordReport(chiffres, rapport.chiffre_carree, rapport.ecart_absolu_moyen,
                             rapport.taille_echantillon, bool(rapport.estnormal))
